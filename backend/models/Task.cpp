#include "Task.h"
#include <cmath>
#include <stdexcept>

// Construct a new task and validate its starting values using the setters.
Task::Task(const std::string& title, const std::string& subject,
           const std::string& deadline, double estimatedHours) {
    setTitle(title);
    setSubject(subject);
    setDeadline(deadline);
    setEstimated(estimatedHours);
    setRemaining(estimatedHours);
}

void Task::setTitle(const std::string& title) {
    if (title.empty() || title.size() > 200)
        throw std::invalid_argument("Task title is required (up to 200 characters).");
    title_ = title;
}

void Task::setDeadline(const std::string& deadline) {
    // Past dates can be loaded from the database. Only the format is checked here.
    if (deadline.size() != 10 || deadline[4] != '-' || deadline[7] != '-')
        throw std::invalid_argument("Expected a date in YYYY-MM-DD format.");
    deadline_ = deadline;
}

void Task::setDifficulty(const std::string& difficulty) {
    if (difficulty != "Easy" && difficulty != "Medium" && difficulty != "Hard")
        throw std::invalid_argument("Invalid task difficulty.");
    difficulty_ = difficulty;
}

void Task::setPriority(const std::string& priority) {
    if (priority != "Low" && priority != "Medium" && priority != "High")
        throw std::invalid_argument("Invalid task priority.");
    priority_ = priority;
}

void Task::setStatus(const std::string& status) {
    if (status != "Pending" && status != "In Progress" && status != "Completed")
        throw std::invalid_argument("Invalid task status.");
    status_ = status;
}

void Task::setEstimated(double hours) {
    if (!std::isfinite(hours) || hours < 0)
        throw std::invalid_argument("Estimated hours must be non-negative.");
    estimated_ = hours;
}

void Task::setRemaining(double hours) {
    if (!std::isfinite(hours) || hours < 0)
        throw std::invalid_argument("Remaining hours must be non-negative.");
    remaining_ = hours;
}

// Larger scores represent higher priority; derived classes add a difficulty weight.
double Task::score() const {
    int points = 1;
    if (priority_ == "High") points = 3;
    else if (priority_ == "Medium") points = 2;
    return points * 10 + weightedDifficulty();
}

// Operator overloading allows two tasks to be compared by score.
bool Task::operator>(const Task& other) const {
    return score() > other.score();
}

std::string Assignment::type() const { return "Assignment"; }
double Assignment::weightedDifficulty() const { return 1.0; }

std::string Project::type() const { return "Project"; }
double Project::weightedDifficulty() const { return 3.0; }

std::string LabWork::type() const { return "Lab Work"; }
double LabWork::weightedDifficulty() const { return 1.5; }

std::string Exam::type() const { return "Exam"; }
double Exam::weightedDifficulty() const { return 2.5; }

std::string Revision::type() const { return "Revision"; }
double Revision::weightedDifficulty() const { return 1.0; }

// Create a concrete task depending on its academic category.
std::unique_ptr<Task> makeTask(const std::string& category) {
    if (category == "Project") return std::make_unique<Project>();
    if (category == "Lab Work") return std::make_unique<LabWork>();
    if (category == "Exam") return std::make_unique<Exam>();
    if (category == "Revision") return std::make_unique<Revision>();
    return std::make_unique<Assignment>();
}
