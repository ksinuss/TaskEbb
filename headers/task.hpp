#ifndef TASK_HPP
#define TASK_HPP

#include <string>
#include <chrono>
#include "periodic_tracker.hpp"
#include <QDateTime>

/**
 * @class Task
 * @brief Stores information about the task: name, description, completion status, repeat interval
 */
class Task {
public:
    enum class Type { OneTime, Deadline, Recurring };

    /**
     * @brief Construct a new Task object
     * @param title Task title (required)
     * @param description Task description (empty by default)
     */
    Task(const std::string& title, 
         const std::string& description = "",
         Type type = Type::OneTime,
         QDateTime deadline = QDateTime(),
         std::chrono::hours interval = std::chrono::hours(0),
         QDateTime endDate = QDateTime()
    );
    
    ~Task() = default;

    /**
     * @brief Get the id object
     * @return Unique task ID in "timestamp_titleprefix_random" format
     */
    std::string get_id() const;

    /**
     * @brief Get the title object
     * @return Task title as a string
     */
    std::string get_title() const;

    /**
     * @brief Get the description object
     * @return Task description as a string
     */
    std::string get_description() const;

    /**
     * @brief Get the interval object
     * @return Interval in hours as std::chrono::hours 
     */
    std::chrono::hours get_interval() const noexcept;

    /**
     * @brief Check task completion status
     * @return True if the task is completed, otherwise false 
     */
    bool is_completed() const;

    /**
     * @brief Set the repetition interval
     * @param interval New interval in hours
     */
    void set_interval(std::chrono::hours interval);

    /**
     * @brief Update task completion status
     * @param status Status true for completed, false for incomplete
     */
    void mark_completed(bool status);
    
    void set_id(const std::string& id);
    void set_title(const std::string& title);
    void set_description(const std::string& description);
    void mark_execution(const PeriodicTracker::TimePoint& timestamp);
    std::chrono::hours get_calculated_interval() const;
    const PeriodicTracker& get_tracker() const noexcept;
    void set_recurring(bool status) noexcept;
    bool is_recurring() const noexcept;
    Type get_type() const noexcept;
    QDateTime get_deadline() const noexcept;
    QDateTime get_end_date() const noexcept;
    void set_deadline(const QDateTime& deadline);
    void set_end_date(const QDateTime& endDate);
    void set_type(Type new_type);
    void set_type(Type new_type, const QDateTime& deadline, std::chrono::hours interval, const QDateTime& end_date);

private:
    char id_[19];                   ///< Unique ID (13 digits + '_' + 4 digits + '\0')
    std::string title_;             ///< Task title
    std::string description_;       ///< Task description
    bool is_completed_;             ///< Completion status
    std::chrono::hours interval_;   ///< Repetition interval in hours
    PeriodicTracker tracker_;
    bool is_recurring_ = false;
    Type type_;
    QDateTime deadline_;
    QDateTime endDate_;
};

#endif