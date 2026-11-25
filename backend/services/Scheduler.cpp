#include "Scheduler.h"

// These helpers keep the main scheduling function short enough to explain.
namespace {

// Return only tasks that still need work, and count their pending hours.
std::vector<const Task*> collectPendingTasks(
    const std::vector<std::unique_ptr<Task>>& tasks,
    double& requiredHours) {
    std::vector<const Task*> pending;
    requiredHours = 0;
    for (const auto& task : tasks) {
        if (task->getStatus() != "Completed" && task->getRemaining() > 0.00001) {
            pending.push_back(task.get());
            requiredHours += task->getRemaining();
        }
    }
    return pending;
}

// Prioritise earlier deadlines. Equal deadlines use the overloaded > operator.
void orderTasks(std::vector<const Task*>& tasks) {
    sortItems(tasks, [](const Task* first, const Task* second) {
        if (first->getDeadline() != second->getDeadline())
            return first->getDeadline() < second->getDeadline();
        return *first > *second;
    });
}

// The timetable covers the current day and the following 29 days.
std::map<std::string, double> makeDailyCapacity(
    const std::string& firstDay,
    const std::array<double, 7>& availableHours) {
    std::map<std::string, double> freeHours;
    for (int day = 0; day < 30; ++day) {
        std::string date = offsetDate(firstDay, day);
        freeHours[date] = availableHours[weekday(date)];
    }
    return freeHours;
}


// First reserve study hours the user moved manually.
void reservePinnedSessions(const std::vector<Scheduler::Pin>& pins,
                       const std::vector<const Task*>& pendingTasks,
                       std::map<int, double>& hoursLeft,
                       std::map<std::string, double>& freeHours,
                       Json::Array& sessions, double& allocatedHours) {
    // Treat tiny rounding differences as zero hours.
    const double minimumHours = 0.00001;
    for (const Scheduler::Pin& pin : pins) {
        auto remaining = hoursLeft.find(pin.taskId);
        if (remaining == hoursLeft.end() || !freeHours.count(pin.date) || pin.hours <= 0) {
            continue;
        }

        const Task* matchingTask = nullptr;
        for (const Task* task : pendingTasks) {
            if (task->getId() == pin.taskId) {
                matchingTask = task;
                break;
            }
        }
        if (matchingTask == nullptr || pin.date > matchingTask->getDeadline()) {
            continue;
        }

        double hours = std::min({pin.hours, remaining->second, freeHours[pin.date]});
        if (hours < minimumHours) {
            continue;
        }
        freeHours[pin.date] -= hours;
        remaining->second -= hours;
        allocatedHours += hours;
        sessions.push_back(obj({
            {"taskId", pin.taskId}, {"title", matchingTask->getTitle()},
            {"date", pin.date}, {"hours", hours},
            {"pinId", pin.id}, {"manual", true}
        }));
    }
}

// Allocate remaining work to the earliest free days before its deadline.
void allocateTaskHours(const Task* task, const std::string& firstDay,
                   std::map<std::string, double>& freeHours,
                   double& remainingHours, Json::Array& sessions, double& allocatedHours) {
    // Treat tiny rounding differences as zero hours.
    const double minimumHours = 0.00001;
    if (task->getDeadline() >= firstDay) {
        for (auto& [date, freeToday] : freeHours) {
            if (date > task->getDeadline() || remainingHours < minimumHours) {
                break;
            }
            double hours = std::min(freeToday, remainingHours);
            if (hours < minimumHours) {
                continue;
            }
            freeToday -= hours;
            remainingHours -= hours;
            allocatedHours += hours;
            sessions.push_back(obj({
                {"taskId", task->getId()}, {"title", task->getTitle()},
                {"date", date}, {"hours", hours},
                {"pinId", 0}, {"manual", false}
            }));
        }
    }

}

// Record why a task was scheduled and whether any hours could not fit.
void explainTaskPlan(const Task* task, double remainingHours,
                 const std::string& firstDay, const std::string& lastDay,
                 Json::Array& risks, Json::Array& explanations,
                 double& unallocatedHours) {
    // Treat tiny rounding differences as zero hours.
    const double minimumHours = 0.00001;
    // STEP 6: Explain the plan, including any hours that did not fit.
    std::string reason = "Deadline-first: due " + task->getDeadline() + ". " +
        task->getPriority() + " priority; " + task->getDifficulty() +
        " difficulty. The scheduler reserves moved sessions, then uses the earliest available study hours.";

    if (remainingHours > minimumHours) {
        unallocatedHours += remainingHours;
        std::string problem;
        if (task->getDeadline() < firstDay) {
            problem = "Deadline has passed";
        } else if (task->getDeadline() > lastDay) {
            problem = "Some work lies beyond the 30-day planning horizon";
        } else {
            problem = "Not enough free study hours before the deadline";
        }
        risks.push_back(obj({
            {"taskId", task->getId()}, {"title", task->getTitle()},
            {"unallocated", remainingHours}, {"reason", problem}
        }));
        reason += " " + problem + "; " + std::to_string(remainingHours) +
                  " hours remain unallocated.";
    }

    explanations.push_back(obj({
        {"taskId", task->getId()}, {"title", task->getTitle()},
        {"deadline", task->getDeadline()}, {"priority", task->getPriority()},
        {"difficulty", task->getDifficulty()},
        {"allocated", task->getRemaining() - remainingHours},
        {"unallocated", remainingHours}, {"reason", reason}
    }));
}

// Keep the final timetable in a predictable order.
void orderSessions(Json::Array& sessions) {
    std::sort(sessions.begin(), sessions.end(), [](const Json& first, const Json& second) {
        if (get(first, "date") != get(second, "date")) {
            return get(first, "date") < get(second, "date");
        }
        return number(first, "taskId") < number(second, "taskId");
    });
}

} // namespace

Json Scheduler::calculate(const std::vector<std::unique_ptr<Task>>& tasks,
               const std::array<double, 7>& availableHours,
               const std::vector<Scheduler::Pin>& pins) const {
    const std::string firstDay = today();
    const std::string lastDay = offsetDate(firstDay, 29);

    // STEP 1: Find all unfinished work.
    double requiredHours = 0;
    auto pendingTasks = collectPendingTasks(tasks, requiredHours);

    // STEP 2: Sort by deadline, then task priority.
    orderTasks(pendingTasks);

    // STEP 3: Create 30 days of available study hours.
    double weeklyCapacity = 0;
    for (double hours : availableHours) weeklyCapacity += hours;
    auto freeHours = makeDailyCapacity(firstDay, availableHours);

    std::map<int, double> hoursLeft;
    for (const Task* task : pendingTasks) {
        hoursLeft[task->getId()] = task->getRemaining();
    }

    Json::Array sessions;
    Json::Array risks;
    Json::Array explanations;
    double allocatedHours = 0;
    double unallocatedHours = 0;

    // STEP 4: Respect manually reserved sessions before auto-allocation.
    reservePinnedSessions(pins, pendingTasks, hoursLeft, freeHours,
                          sessions, allocatedHours);

    // STEP 5: Place remaining work and explain each task's result.
    for (const Task* task : pendingTasks) {
        double& remainingHours = hoursLeft[task->getId()];
        allocateTaskHours(task, firstDay, freeHours, remainingHours, sessions, allocatedHours);
        explainTaskPlan(task, remainingHours, firstDay, lastDay, risks,
                        explanations, unallocatedHours);
    }

    orderSessions(sessions);

    // Workload percentage compares pending work with one week of available study hours.
    double utilization = weeklyCapacity > 0
        ? 100.0 * requiredHours / weeklyCapacity
        : (requiredHours ? 101.0 : 0.0);

    return obj({
        {"sessions", Json(sessions)}, {"risks", Json(risks)},
        {"explanations", Json(explanations)},
        {"required", requiredHours}, {"allocated", allocatedHours},
        {"unallocated", unallocatedHours},
        {"weeklyCapacity", weeklyCapacity}, {"utilization", utilization}
    });
}
