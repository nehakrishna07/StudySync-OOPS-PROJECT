#pragma once

#include "../models/Task.h"
#include "../utils/Common.h"

// Makes a study plan using a simple rule: tasks due sooner come first.
// All calculations happen in memory; this class does not change the database.
class Scheduler {
public:
    // A session the student has manually moved to a particular date.
    struct Pin {
        int id;
        int taskId;
        std::string date;
        double hours;
    };

    Json calculate(const std::vector<std::unique_ptr<Task>>& tasks,
                   const std::array<double, 7>& availableHours,
                   const std::vector<Pin>& pins = {}) const;
};
