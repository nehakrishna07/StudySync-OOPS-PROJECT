#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// Abstract class. Task details are private and can be read through getters.
class Task {
private:
    int id_ = 0;
    std::string title_;
    std::string subject_;
    std::string category_;
    std::string description_;
    std::string deadline_;
    std::string difficulty_ = "Medium";
    std::string priority_ = "Medium";
    std::string status_ = "Pending";
    double estimated_ = 0;
    double remaining_ = 0;

public:
    Task() = default;
    Task(const std::string& title, const std::string& subject,
         const std::string& deadline, double estimatedHours);
    virtual ~Task() = default;

    int getId() const { return id_; }
    const std::string& getTitle() const { return title_; }
    const std::string& getSubject() const { return subject_; }
    const std::string& getCategory() const { return category_; }
    const std::string& getDescription() const { return description_; }
    const std::string& getDeadline() const { return deadline_; }
    const std::string& getDifficulty() const { return difficulty_; }
    const std::string& getPriority() const { return priority_; }
    const std::string& getStatus() const { return status_; }
    double getEstimated() const { return estimated_; }
    double getRemaining() const { return remaining_; }

    void setId(int id) { id_ = id; }
    void setTitle(const std::string& title);
    void setSubject(const std::string& subject) { subject_ = subject; }
    void setCategory(const std::string& category) { category_ = category; }
    void setDescription(const std::string& description) { description_ = description; }
    void setDeadline(const std::string& deadline);
    void setDifficulty(const std::string& difficulty);
    void setPriority(const std::string& priority);
    void setStatus(const std::string& status);
    void setEstimated(double hours);
    void setRemaining(double hours);

    virtual std::string type() const = 0;
    virtual double weightedDifficulty() const = 0;
    double score() const;
    bool operator>(const Task& other) const;
};

// Inheritance: each concrete task supplies its own type and difficulty weight.
struct Assignment : Task {
    using Task::Task;
    std::string type() const override;
    double weightedDifficulty() const override;
};
struct Project : Task {
    using Task::Task;
    std::string type() const override;
    double weightedDifficulty() const override;
};
struct LabWork : Task {
    using Task::Task;
    std::string type() const override;
    double weightedDifficulty() const override;
};
struct Exam : Task {
    using Task::Task;
    std::string type() const override;
    double weightedDifficulty() const override;
};
struct Revision : Task {
    using Task::Task;
    std::string type() const override;
    double weightedDifficulty() const override;
};

// Factory: create the right derived object based on task category.
std::unique_ptr<Task> makeTask(const std::string& category);

// Generic template: sort any vector with a supplied comparison rule.
template <class T, class Comparator>
void sortItems(std::vector<T>& items, Comparator compare) {
    std::sort(items.begin(), items.end(), compare);
}
