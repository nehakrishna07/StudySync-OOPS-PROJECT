#pragma once
#include "../database/Database.h"

// Reads tasks from SQLite and converts them into the correct C++ task types.
class TaskManager {
private:
    Database& database;

public:
    explicit TaskManager(Database& db);

    std::vector<std::unique_ptr<Task>> list(int userId);

    // Convert a C++ Task into the JSON fields expected by JavaScript.
    Json asJson(const Task& task);

    // Function overloading: search by numeric ID or by task title.
    std::unique_ptr<Task> findTask(int userId, int taskId);
    std::unique_ptr<Task> findTask(int userId, const std::string& title);
};
