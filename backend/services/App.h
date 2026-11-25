#pragma once
#include "TaskManager.h"
#include "Scheduler.h"
// Routes /api/... requests to authentication, tasks, availability and analysis.
class App
{
    Database db_;
    TaskManager tasks_;
    Scheduler scheduler_;
    std::unordered_map<std::string,int> sessions_;
    // Load the student's usual study hours (Monday = 0, Sunday = 6).
    std::array<double, 7> availability(int userId);

    // Read only the current user's manually placed study sessions.
    std::vector<Scheduler::Pin> pins(int userId);
    Json plan(int userId);
    Json alerts(int userId);
    Json::Array getWorkloadAlerts(int userId);
    Json::Array getDeadlineAlerts(int userId);
    // Check task details before saving them. The frontend also validates
    // forms, but the C++ backend must never rely on browser checks alone.
    static bool taskValid(const Json& task, std::string& message);

    // Small request handlers keep the main router easy to follow.
    Json registerUser(const Json& body, int& status);
    Json loginUser(const Json& body, int& status);
    static bool validRegistration(const std::string& name, const std::string& email,
                                  const std::string& password);
    Json createUser(const std::string& name, const std::string& email,
                    const std::string& password, int& status);
    Json taskRoutes(const std::string& method, const std::string& path, const Json& body, int userId, int& status);
    // Task operations are separate so adding/editing/deleting are easy to find.
    Json listTasks(int userId);
    Json addTask(const Json& body, int userId, int& status);
    Json editTask(int taskId, const Json& body, int userId, int& status);
    Json deleteTask(int taskId, int userId, int& status);
    Json updateTaskProgress(int taskId, const Json& body, int userId, int& status);
    Json getAvailability(int userId);
    Json saveAvailability(const Json& body, int userId, int& status);
    Json availabilityRoutes(const std::string& method, const std::string& path, const Json& body, int userId, int& status);
    Json simulateRoute(const std::string& method, const std::string& path, const Json& body, int userId, int& status);
    // Calendar: check the request, check available hours, then save the session.
    bool validateSessionMove(const Json& body, int& status, std::string& message);
    bool checkSessionCapacity(int userId, int taskId, const std::string& day,
                              double hours, int& status, std::string& message);
    void savePinnedSession(int userId, int taskId, const std::string& day, double hours);
    Json moveSession(const Json& body, int userId, int& status);
    Json unpinSession(const Json& body, int userId);
    Json calendarRoutes(const std::string& method, const std::string& path, const Json& body, int userId, int& status);
    Json getTimeLogs(int userId);
    // Time tracking: validate minutes and keep both database changes together.
    bool validateTimeLog(const Json& body, int& status, std::string& message);
    bool taskExists(int userId, int taskId);
    void recordStudyTime(int userId, int taskId, int minutes);
    void updateTaskProgressAfterStudy(int userId, int taskId, int minutes);
    Json saveTimeLog(const Json& body, int userId, int& status);
    Json timeRoutes(const std::string& method, const std::string& path, const Json& body, int userId, int& status);
    Json reminderRoutes(const std::string& method, const std::string& path, const Json& body, int userId, int& status);


public:
    explicit App(const fs::path& p);
    Json handle(const std::string& method, const std::string& path,
                const Json& body, const std::string& bearer, int& status);
};
