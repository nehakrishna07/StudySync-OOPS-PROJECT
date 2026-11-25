#include "App.h"

// Route browser requests to the correct feature. Protected routes require a session token.
Json App::handle(const std::string& method, const std::string& path,
                 const Json& body, const std::string& bearer, int& status) {
    // Public endpoints do not require a login token.
    if (path == "/api/register" && method == "POST")
        return registerUser(body, status);
    if (path == "/api/login" && method == "POST")
        return loginUser(body, status);

    auto session = sessions_.find(bearer);
    if (session == sessions_.end()) {
        status = 401;
        return err("Please sign in.");
    }
    int userId = session->second;

    if (path == "/api/logout" && method == "POST") {
        sessions_.erase(bearer);
        return obj({{"ok", true}});
    }
    if (path == "/api/tasks" || path.rfind("/api/tasks/", 0) == 0)
        return taskRoutes(method, path, body, userId, status);
    if (path == "/api/availability")
        return availabilityRoutes(method, path, body, userId, status);
    if (path == "/api/simulate" && method == "POST")
        return simulateRoute(method, path, body, userId, status);
    if (path == "/api/analysis" && method == "GET")
        return plan(userId);
    if (path == "/api/schedule/move" || path == "/api/schedule/unpin")
        return calendarRoutes(method, path, body, userId, status);
    if (path == "/api/time-logs")
        return timeRoutes(method, path, body, userId, status);
    if (path == "/api/reminders" && method == "GET")
        return alerts(userId);
    if (path == "/api/reminders/dismiss" && method == "POST")
        return reminderRoutes(method, path, body, userId, status);

    status = 404;
    return err("Unknown API endpoint");
}

Json App::registerUser(const Json& body, int& status) {
    const std::string name = get(body, "name");
    std::string email = get(body, "email");
    const std::string password = get(body, "password");
    std::transform(email.begin(), email.end(), email.begin(),
                   [](unsigned char ch) { return std::tolower(ch); });
    if (!validRegistration(name, email, password)) {
        status = 400;
        return err("Enter a name, valid email, and password of at least 8 characters.");
    }
    return createUser(name, email, password, status);
}

bool App::validRegistration(const std::string& name, const std::string& email,
                            const std::string& password) {
    return !name.empty() && name.size() <= 100 && email.size() <= 254 &&
           email.find('@') != std::string::npos &&
           password.size() >= 8 && password.size() <= 1024;
}

Json App::createUser(const std::string& name, const std::string& email,
                     const std::string& password, int& status) {
    const std::string salt = hex(randomBytes(16).data(), 16);
    const std::string hash = passwordHash(password, salt);
    try {
        Database::Statement query(db_.raw(),
            "INSERT INTO users(name,email,salt,hash) VALUES(?,?,?,?)");
        query.bind(1, name);
        query.bind(2, email);
        query.bind(3, salt);
        query.bind(4, hash);
        query.run();
    } catch (const std::exception&) {
        status = 409;
        return err("Email already registered or database error.");
    }
    const int userId = db_.inserted();
    for (int day = 0; day < 7; ++day) {
        Database::Statement query(db_.raw(),
            "INSERT INTO availability(user_id,weekday,hours) VALUES(?,?,?)");
        query.bind(1, userId);
        query.bind(2, day);
        query.bind(3, day >= 5 ? 3.0 : 2.0);
        query.run();
    }
    return obj({{"ok", true}});
}


// Check the password and create a login token for the current session.
Json App::loginUser(const Json& body, int& status) {
    std::string email = get(body, "email");
    std::transform(email.begin(), email.end(), email.begin(),
                   [](unsigned char character) { return std::tolower(character); });

    Database::Statement userQuery(db_.raw(),
        "SELECT id,name,salt,hash FROM users WHERE email=?");
    userQuery.bind(1, email);

    // Check the saved password hash, not the original password.
    if (!userQuery.next() ||
        !secureEqual(passwordHash(get(body, "password"), userQuery.text(2)),
                     userQuery.text(3))) {
        status = 401;
        return err("Invalid email or password.");
    }

    const auto randomTokenBytes = randomBytes(32);
    const std::string loginToken = hex(randomTokenBytes.data(), randomTokenBytes.size());
    sessions_[loginToken] = userQuery.integer(0);

    return obj({{"token", loginToken}, {"name", userQuery.text(1)}});
}

// The router only selects an action. Each action is implemented below.
Json App::taskRoutes(const std::string& method, const std::string& path,
                     const Json& body, int userId, int& status) {
    if (path == "/api/tasks") {
        if (method == "GET") return listTasks(userId);
        if (method == "POST") return addTask(body, userId, status);
    }
    if (path.rfind("/api/tasks/", 0) == 0 &&
        (method == "PUT" || method == "DELETE")) {
        const std::string idText = path.substr(11);
        if (idText.empty() || idText.find_first_not_of("0123456789") != std::string::npos) {
            status = 400;
            return err("Invalid task ID");
        }
        // Preserve the old route's integer task ID behaviour.
        const int taskId = std::stoi(idText);
        if (method == "DELETE") return deleteTask(taskId, userId, status);
        if (body.at("title").isObject() || !get(body, "title").empty())
            return editTask(taskId, body, userId, status);
        return updateTaskProgress(taskId, body, userId, status);
    }
    status = 404;
    return err("Unknown API endpoint");
}

Json App::listTasks(int userId) {
    Json::Array result;
    for (const auto& task : tasks_.list(userId))
        result.push_back(tasks_.asJson(*task));
    return obj({{"tasks", Json(result)}});
}

Json App::addTask(const Json& body, int userId, int& status) {
    std::string reason;
    if (!taskValid(body, reason)) {
        status = 400;
        return err(reason);
    }
    const double hours = number(body, "estimated");
    Database::Statement query(db_.raw(),
        "INSERT INTO tasks(user_id,title,subject,category,description,deadline,estimated,remaining,difficulty,priority,status) VALUES(?,?,?,?,?,?,?,?,?,?,?)");
    query.bind(1, userId);
    query.bind(2, get(body, "title"));
    query.bind(3, get(body, "subject", "General"));
    query.bind(4, get(body, "category", "Assignment"));
    query.bind(5, get(body, "description"));
    query.bind(6, get(body, "deadline"));
    query.bind(7, hours);
    query.bind(8, hours);
    query.bind(9, get(body, "difficulty", "Medium"));
    query.bind(10, get(body, "priority", "Medium"));
    query.bind(11, std::string("Pending"));
    query.run();
    return obj({{"ok", true}, {"id", db_.inserted()}});
}

Json App::deleteTask(int taskId, int userId, int& status) {
    Database::Statement query(db_.raw(), "DELETE FROM tasks WHERE id=? AND user_id=?");
    query.bind(1, taskId);
    query.bind(2, userId);
    query.run();
    const bool deleted = db_.changed() > 0;
    if (!deleted) status = 404;
    return obj({{"ok", deleted}});
}

Json App::editTask(int taskId, const Json& body, int userId, int& status) {
    std::string reason;
    if (!taskValid(body, reason)) {
        status = 400;
        return err(reason);
    }
    const double hours = number(body, "estimated");
    const double remaining = number(body, "remaining", hours);
    if (remaining < 0 || remaining > hours) {
        status = 400;
        return err("Invalid remaining hours");
    }
    Database::Statement query(db_.raw(),
        "UPDATE tasks SET title=?,subject=?,category=?,description=?,deadline=?,estimated=?,remaining=?,difficulty=?,priority=?,status=? WHERE id=? AND user_id=?");
    query.bind(1, get(body, "title"));
    query.bind(2, get(body, "subject", "General"));
    query.bind(3, get(body, "category", "Assignment"));
    query.bind(4, get(body, "description"));
    query.bind(5, get(body, "deadline"));
    query.bind(6, hours);
    query.bind(7, remaining);
    query.bind(8, get(body, "difficulty", "Medium"));
    query.bind(9, get(body, "priority", "Medium"));
    query.bind(10, remaining == 0 ? "Completed" : get(body, "status", "Pending"));
    query.bind(11, taskId);
    query.bind(12, userId);
    query.run();
    return obj({{"ok", db_.changed() > 0}});
}

Json App::updateTaskProgress(int taskId, const Json& body, int userId, int& status) {
    const std::string progress = get(body, "status");
    const double remaining = number(body, "remaining", -1);
    if (!in(progress, {"Pending", "In Progress", "Completed"}) ||
        !body.at("remaining").isNumber() || remaining < 0) {
        status = 400;
        return err("Invalid status or remaining hours");
    }
    Database::Statement query(db_.raw(),
        "UPDATE tasks SET status=?,remaining=? WHERE id=? AND user_id=? AND estimated>=?");
    query.bind(1, progress);
    query.bind(2, progress == "Completed" ? 0.0 : remaining);
    query.bind(3, taskId);
    query.bind(4, userId);
    query.bind(5, remaining);
    query.run();
    const bool updated = db_.changed() > 0;
    if (!updated) status = 404;
    return obj({{"ok", updated}});
}

Json App::availabilityRoutes(const std::string& method, const std::string& path,
                             const Json& body, int userId, int& status) {
    if (path == "/api/availability") {
        if (method == "GET") return getAvailability(userId);
        if (method == "PUT") return saveAvailability(body, userId, status);
    }
    status = 404;
    return err("Unknown API endpoint");
}

Json App::getAvailability(int userId) {
    Json::Array values;
    for (double hours : availability(userId)) values.push_back(hours);
    return obj({{"hours", Json(values)}});
}

// Save all seven days together, so a failed update does not leave partial data.
Json App::saveAvailability(const Json& body, int userId, int& status) {
    const Json& values = body.at("hours");
    if (!values.isArray() || values.array().size() != 7) {
        status = 400;
        return err("Seven daily values required");
    }
    for (const auto& hours : values.array()) {
        if (!hours.isNumber() || hours.num() < 0 || hours.num() > 24) {
            status = 400;
            return err("Daily hours must be between 0 and 24");
        }
    }

    db_.execute("BEGIN IMMEDIATE");
    try {
        for (int day = 0; day < 7; ++day) {
            Database::Statement query(db_.raw(),
                "INSERT INTO availability(user_id,weekday,hours) VALUES(?,?,?) ON CONFLICT(user_id,weekday) DO UPDATE SET hours=excluded.hours");
            query.bind(1, userId);
            query.bind(2, day);
            query.bind(3, values.array()[day].num());
            query.run();
        }
        db_.execute("COMMIT");
    } catch (...) {
        db_.execute("ROLLBACK");
        throw;
    }
    return obj({{"ok", true}});
}

// Try adding a task to the schedule without saving it in the database.
Json App::simulateRoute(const std::string& method, const std::string& path,
                        const Json& body, int userId, int& status) {
    if (path != "/api/simulate" || method != "POST") {
        status = 404;
        return err("Unknown API endpoint");
    }

    std::string validationMessage;
    if (!taskValid(body, validationMessage)) {
        status = 400;
        return err(validationMessage);
    }

    auto simulatedTasks = tasks_.list(userId);
    auto newTask = makeTask(get(body, "category", "Assignment"));
    newTask->setId(-1); // A temporary ID: this task is never inserted into SQLite.
    newTask->setTitle(get(body, "title"));
    newTask->setSubject(get(body, "subject", "General"));
    newTask->setDeadline(get(body, "deadline"));
    newTask->setEstimated(number(body, "estimated"));
    newTask->setRemaining(number(body, "estimated"));
    newTask->setPriority(get(body, "priority", "Medium"));
    newTask->setDifficulty(get(body, "difficulty", "Medium"));
    simulatedTasks.push_back(std::move(newTask));

    const Json currentSchedule = plan(userId);
    const Json simulatedSchedule = scheduler_.calculate(
        simulatedTasks, availability(userId), pins(userId));
    return obj({{"before", currentSchedule}, {"after", simulatedSchedule},
                {"saved", false}});
}

Json App::calendarRoutes(const std::string& method, const std::string& path,
                         const Json& body, int userId, int& status) {
    if (path == "/api/schedule/move" && method == "POST")
        return moveSession(body, userId, status);
    if (path == "/api/schedule/unpin" && method == "POST")
        return unpinSession(body, userId);
    status = 404;
    return err("Unknown API endpoint");
}

bool App::validateSessionMove(const Json& body, int& status, std::string& message) {
    const std::string day = get(body, "date");
    const double hours = number(body, "hours");

    if (!validDate(day) || day < today() || day > offsetDate(today(), 29) ||
        !body.at("hours").isNumber() || !std::isfinite(hours) || hours <= 0) {
        status = 400;
        message = "Choose a future date within 30 days and positive hours.";
        return false;
    }
    return true;
}

// A moved session must fit both the task workload and the day's study limit.
bool App::checkSessionCapacity(int userId, int taskId, const std::string& day,
                               double hours, int& status, std::string& message) {
    // Only look at tasks owned by this user.
    const auto allTasks = tasks_.list(userId);
    const Task* chosen = nullptr;
    for (const auto& task : allTasks) {
        if (task->getId() == taskId && task->getStatus() != "Completed") {
            chosen = task.get();
            break;
        }
    }

    if (!chosen) {
        status = 404;
        message = "Task not found.";
        return false;
    }
    if (day > chosen->getDeadline()) {
        status = 400;
        message = "Cannot schedule after the task deadline.";
        return false;
    }

    double hoursForTask = 0;
    double hoursForDay = 0;
    for (const auto& session : pins(userId)) {
        if (session.taskId == taskId) hoursForTask += session.hours;
        if (session.date == day) hoursForDay += session.hours;
    }

    const double dayLimit = availability(userId)[weekday(day)];
    if (hours + hoursForTask > chosen->getRemaining() + 0.00001 ||
        hours + hoursForDay > dayLimit + 0.00001) {
        status = 400;
        message = "Pinned hours exceed this task's remaining work or the day's capacity.";
        return false;
    }
    return true;
}

void App::savePinnedSession(int userId, int taskId, const std::string& day, double hours) {
    Database::Statement query(db_.raw(),
        "INSERT INTO pinned_sessions(user_id,task_id,day,hours) VALUES(?,?,?,?)");
    query.bind(1, userId);
    query.bind(2, taskId);
    query.bind(3, day);
    query.bind(4, hours);
    query.run();
}

Json App::moveSession(const Json& body, int userId, int& status) {
    std::string message;
    if (!validateSessionMove(body, status, message)) return err(message);

    const int taskId = int(number(body, "taskId"));
    const std::string day = get(body, "date");
    const double hours = number(body, "hours");
    if (!checkSessionCapacity(userId, taskId, day, hours, status, message))
        return err(message);

    savePinnedSession(userId, taskId, day, hours);
    return obj({{"ok", true}, {"analysis", plan(userId)}});
}

Json App::unpinSession(const Json& body, int userId) {
    const int pinId = int(number(body, "pinId"));
    Database::Statement query(db_.raw(),
        "DELETE FROM pinned_sessions WHERE id=? AND user_id=?");
    query.bind(1, pinId);
    query.bind(2, userId);
    query.run();
    return obj({{"ok", db_.changed() > 0}});
}


Json App::timeRoutes(const std::string& method, const std::string& path,
                     const Json& body, int userId, int& status) {
    if (path == "/api/time-logs" && method == "GET")
        return getTimeLogs(userId);
    if (path == "/api/time-logs" && method == "POST")
        return saveTimeLog(body, userId, status);
    status = 404;
    return err("Unknown API endpoint");
}

// Return recent study sessions and the total minutes logged per task.
Json App::getTimeLogs(int userId) {
    Database::Statement query(db_.raw(),
        "SELECT l.id,l.task_id,t.title,l.minutes,l.logged_at FROM time_logs l "
        "JOIN tasks t ON l.task_id=t.id AND t.user_id=l.user_id "
        "WHERE l.user_id=? ORDER BY l.id DESC LIMIT 100");
    query.bind(1, userId);
    Json::Array logs;
    while (query.next()) {
        logs.push_back(obj({{"id", query.integer(0)}, {"taskId", query.integer(1)},
            {"title", query.text(2)}, {"minutes", query.integer(3)},
            {"loggedAt", query.text(4)}}));
    }
    Database::Statement totals(db_.raw(),
        "SELECT task_id,SUM(minutes) FROM time_logs WHERE user_id=? GROUP BY task_id");
    totals.bind(1, userId);
    Json::Array summary;
    while (totals.next())
        summary.push_back(obj({{"taskId", totals.integer(0)},
                               {"minutes", totals.integer(1)}}));
    return obj({{"logs", Json(logs)}, {"totals", Json(summary)}});
}

bool App::validateTimeLog(const Json& body, int& status, std::string& message) {
    const double minutes = number(body, "minutes");
    if (!body.at("minutes").isNumber() || !std::isfinite(minutes) ||
        minutes < 1 || minutes > 1440 || std::floor(minutes) != minutes) {
        status = 400;
        message = "Study minutes must be a whole number from 1 to 1440.";
        return false;
    }
    return true;
}

bool App::taskExists(int userId, int taskId) {
    Database::Statement query(db_.raw(),
        "SELECT remaining FROM tasks WHERE id=? AND user_id=?");
    query.bind(1, taskId);
    query.bind(2, userId);
    return query.next();
}

void App::recordStudyTime(int userId, int taskId, int minutes) {
    Database::Statement query(db_.raw(),
        "INSERT INTO time_logs(user_id,task_id,minutes) VALUES(?,?,?)");
    query.bind(1, userId);
    query.bind(2, taskId);
    query.bind(3, minutes);
    query.run();
}

void App::updateTaskProgressAfterStudy(int userId, int taskId, int minutes) {
    // Study time is recorded in minutes; remaining task work is stored in hours.
    const double hoursStudied = minutes / 60.0;
    Database::Statement query(db_.raw(),
        "UPDATE tasks SET remaining=MAX(0,remaining-?),"
        "status=CASE WHEN remaining-?<=0 THEN 'Completed' ELSE 'In Progress' END "
        "WHERE id=? AND user_id=?");
    query.bind(1, hoursStudied);
    query.bind(2, hoursStudied);
    query.bind(3, taskId);
    query.bind(4, userId);
    query.run();
}

// The study log and task progress update must be saved together.
Json App::saveTimeLog(const Json& body, int userId, int& status) {
    std::string message;
    if (!validateTimeLog(body, status, message)) return err(message);

    const int taskId = int(number(body, "taskId"));
    const int minutes = int(number(body, "minutes"));
    if (!taskExists(userId, taskId)) {
        status = 404;
        return err("Task not found.");
    }

    // Both changes must succeed or both must be undone.
    db_.execute("BEGIN IMMEDIATE");
    try {
        recordStudyTime(userId, taskId, minutes);
        updateTaskProgressAfterStudy(userId, taskId, minutes);
        db_.execute("COMMIT");
    } catch (...) {
        db_.execute("ROLLBACK");
        throw;
    }
    return obj({{"ok", true}, {"analysis", plan(userId)}});
}

// Hide a reminder for this user without deleting the underlying task.
Json App::reminderRoutes(const std::string& method, const std::string& path,
                         const Json& body, int userId, int& status) {
    if (path != "/api/reminders/dismiss" || method != "POST") {
        status = 404;
        return err("Unknown API endpoint");
    }

    const std::string reminderKey = get(body, "key");
    if (reminderKey.empty() || reminderKey.size() > 200) {
        status = 400;
        return err("Invalid reminder key.");
    }

    Database::Statement query(db_.raw(),
        "INSERT OR IGNORE INTO dismissed_alerts(user_id,alert_key) VALUES(?,?)");
    query.bind(1, userId);
    query.bind(2, reminderKey);
    query.run();
    return obj({{"ok", true}});
}

App::App(const fs::path& databasePath) : db_(databasePath), tasks_(db_) {}

// All task forms use the same validation rules before changes are saved.
bool App::taskValid(const Json& task, std::string& message) {
    const std::string title = get(task, "title");
    const std::string deadline = get(task, "deadline");
    const std::string category = get(task, "category", "Assignment");
    const std::string priority = get(task, "priority", "Medium");
    const std::string difficulty = get(task, "difficulty", "Medium");
    const double estimatedHours = number(task, "estimated");

    const bool valid = !title.empty() && title.size() <= 200 &&
        get(task, "subject").size() <= 200 &&
        get(task, "description").size() <= 4000 &&
        task.at("estimated").isNumber() && std::isfinite(estimatedHours) &&
        estimatedHours > 0 && estimatedHours <= 10000 &&
        validDate(deadline) && deadline >= today() &&
        in(category, {"Assignment", "Project", "Lab Work", "Exam", "Revision"}) &&
        in(priority, {"Low", "Medium", "High"}) &&
        in(difficulty, {"Easy", "Medium", "Hard"});

    if (!valid) {
        message = "Check task title, deadline, hours, category, priority and difficulty.";
        return false;
    }
    return true;
}

Json App::alerts(int userId) {
    Json::Array items = getWorkloadAlerts(userId);
    Json::Array deadlineAlerts = getDeadlineAlerts(userId);
    for (const auto& item : deadlineAlerts)
        items.push_back(item);

    Database::Statement query(db_.raw(),
        "SELECT alert_key FROM dismissed_alerts WHERE user_id=?");
    query.bind(1, userId);
    std::vector<std::string> dismissed;
    while (query.next()) dismissed.push_back(query.text(0));

    Json::Array active;
    for (const auto& item : items) {
        const std::string key = get(item, "key");
        if (std::find(dismissed.begin(), dismissed.end(), key) == dismissed.end())
            active.push_back(item);
    }
    return obj({{"alerts", Json(active)}});
}

Json::Array App::getWorkloadAlerts(int userId) {
    Json::Array items;
    const Json analysis = plan(userId);
    // Alert keys stay stable after the schedule is recalculated.
    for (const auto& risk : analysis.at("risks").array()) {
        const int taskId = int(number(risk, "taskId"));
        const std::string key = "risk:" + std::to_string(taskId);
        items.push_back(obj({{"key", key}, {"title", get(risk, "title")},
            {"message", get(risk, "reason") + ": " +
                        std::to_string(number(risk, "unallocated")) + "h unallocated"},
            {"level", "warning"}}));
    }
    return items;
}

Json::Array App::getDeadlineAlerts(int userId) {
    Json::Array items;
    const std::string currentDay = today();
    for (const auto& task : tasks_.list(userId)) {
        if (task->getStatus() == "Completed") continue;
        if (task->getDeadline() < currentDay ||
            task->getDeadline() <= offsetDate(currentDay, 3)) {
            const bool overdue = task->getDeadline() < currentDay;
            items.push_back(obj({
                {"key", "deadline:" + std::to_string(task->getId()) + ":" + task->getDeadline()},
                {"title", task->getTitle()},
                {"message", overdue ? "Overdue since " + task->getDeadline()
                                    : "Due soon on " + task->getDeadline()},
                {"level", overdue ? "danger" : "info"}
            }));
        }
    }
    return items;
}


// Run the scheduler using this user's saved tasks and study hours.
Json App::plan(int userId) {
    return scheduler_.calculate(tasks_.list(userId), availability(userId), pins(userId));
}

// Read sessions that the user has manually placed on the calendar.
std::vector<Scheduler::Pin> App::pins(int userId) {
    Database::Statement query(db_.raw(),
        "SELECT id,task_id,day,hours FROM pinned_sessions WHERE user_id=? ORDER BY id");
    query.bind(1, userId);

    std::vector<Scheduler::Pin> pinnedSessions;
    while (query.next()) {
        pinnedSessions.push_back({
            query.integer(0), query.integer(1), query.text(2), query.real(3)
        });
    }
    return pinnedSessions;
}

// Return the seven saved study-hour values (one for each weekday).
std::array<double, 7> App::availability(int userId) {
    std::array<double, 7> studyHours{};
    Database::Statement query(db_.raw(),
        "SELECT weekday,hours FROM availability WHERE user_id=?");
    query.bind(1, userId);

    while (query.next()) {
        const int weekdayNumber = query.integer(0);
        if (weekdayNumber >= 0 && weekdayNumber < 7) {
            studyHours[weekdayNumber] = query.real(1);
        }
    }
    return studyHours;
}
