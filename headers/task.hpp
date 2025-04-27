#ifndef TASK_HPP
#define TASK_HPP

#include <string>
#include <chrono>

/**
 * @class Task
 * @brief Stores information about the task: name, description, completion status, repeat interval
 */
class Task {
public:
    /**
     * @brief Construct a new Task object
     * @param title Task title (required)
     * @param description Task description (empty by default)
     */
    Task(const std::string& title, const std::string& description = "");
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
    std::chrono::hours get_interval() const;

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

private:
    char id_[19];                   ///< Unique ID (13 digits + '_' + 4 digits + '\0')
    std::string title_;             ///< Task title
    std::string description_;       ///< Task description
    bool is_completed_;             ///< Completion status
    std::chrono::hours interval_;   ///< Repetition interval in hours
};

#endif