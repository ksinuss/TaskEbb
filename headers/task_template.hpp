#ifndef TASK_TEMPLATE_HPP
#define TASK_TEMPLATE_HPP

#include "task.hpp"
#include <ctime>
#include <vector>
#include <stdexcept>

/**
 * @class TaskTemplate
 * @brief Generates recurring tasks based on specified intervals
 * 
 */
class TaskTemplate {
public:
    ///< Recurrence types supported by the template
    enum class RecurrenceType { DAILY, WEEKLY, CUSTOM };

    enum TemplateType {
        PERIODIC = Task::Recurring,
        DEADLINE_DRIVEN = Task::Deadline
    };

    /**
     * @brief Construct a new Task Template object
     * @param base_task Prototype task to clone
     * @param type Recurrence pattern type (default = DAILY)
     * @param interval Custom interval in hours (for CUSTOM type, default = 24)
     * @throws std::invalid_argument if interval <= 0 for CUSTOM type
     */
    TaskTemplate(const std::string& title, const std::string& description, TemplateType type, int interval);

    TaskTemplate(const std::string& title, const std::string& description, int interval_hours);

    /**
     * @brief Creating tasks before the specified time stamp
     * @param up_to_timestamp Timestamp for the end of the period (in seconds)
     * @return Vector of generated Task objects
     */
    std::vector<Task> generate_tasks(time_t up_to_timestamp) const;

    /**
     * @brief Get the last time tasks were generated from template
     */
    time_t get_last_generation_time() const noexcept;

    /**
     * @brief Get the base task used as a prototype
     */
    Task get_base_task() const noexcept;

    /**
     * @brief Get the recurrence type object
     */
    RecurrenceType get_recurrence_type() const noexcept;

    /**
     * @brief Get the custom interval hours object (for CUSTOM type)
     */
    int get_custom_interval_hours() const noexcept;

    std::string get_title() const;
    std::string get_description() const;
    int get_interval_hours() const;

private:
    std::string title_;
    std::string description_;
    TemplateType template_type_;
    int interval_hours_;
    Task base_task_;
    RecurrenceType recurrence_type_;
    int custom_interval_hours_;
    mutable time_t last_generated_;     ///< Mutable to allow updating during const generate_tasks()
    time_t get_step() const;
};

#endif