#include "TaskManager.h"

Json TaskManager::asJson(const Task& task) {
    return obj({
        {"id", task.getId()}, {"title", task.getTitle()},
        {"subject", task.getSubject()}, {"category", task.getCategory()},
        {"description", task.getDescription()}, {"deadline", task.getDeadline()},
        {"estimated", task.getEstimated()}, {"remaining", task.getRemaining()},
        {"difficulty", task.getDifficulty()}, {"priority", task.getPriority()},
        {"status", task.getStatus()}
    });
}

std::vector<std::unique_ptr<Task>> TaskManager::list(int userId) {
    const std::string sql =
        "SELECT id,title,subject,category,description,deadline,"
        "estimated,remaining,difficulty,priority,status "
        "FROM tasks WHERE user_id=? ORDER BY deadline,id";

    Database::Statement query(database.raw(), sql);
    query.bind(1, userId);
    std::vector<std::unique_ptr<Task>> userTasks;

    while (query.next()) {
        // makeTask creates Assignment, Project, Exam, etc. as needed.
        auto task = makeTask(query.text(3));
        task->setId(query.integer(0));
        task->setTitle(query.text(1));
        task->setSubject(query.text(2));
        task->setCategory(query.text(3));
        task->setDescription(query.text(4));
        task->setDeadline(query.text(5));
        task->setEstimated(query.real(6));
        task->setRemaining(query.real(7));
        task->setDifficulty(query.text(8));
        task->setPriority(query.text(9));
        task->setStatus(query.text(10));
        userTasks.push_back(std::move(task));
}
        return userTasks;
    }

TaskManager::TaskManager(Database& db) : database(db) {}

// Both methods share the name findTask, but accept different input types.
std::unique_ptr<Task> TaskManager::findTask(int userId, int taskId) {
    auto userTasks = list(userId);
    for (auto& task : userTasks) {
        if (task->getId() == taskId) return std::move(task);
    }
    return nullptr;
}

std::unique_ptr<Task> TaskManager::findTask(int userId, const std::string& title) {
    auto userTasks = list(userId);
    for (auto& task : userTasks) {
        if (task->getTitle() == title) return std::move(task);
    }
    return nullptr;
}
