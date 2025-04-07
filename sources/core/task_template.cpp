#include "task_template.hpp"
#include <cmath>

TaskTemplate::TaskTemplate(const Task& base_task, RecurrenceType type, int interval)
    : base_task_(base_task), recurrence_type_(type), custom_interval_hours_(interval), last_generated_(0)
{
    if (recurrence_type_ == RecurrenceType::CUSTOM && interval <= 0) {
        throw std::invalid_argument("Custom interval must be positive");
    }
}

std::vector<Task> TaskTemplate::generate_tasks(time_t up_to_timestamp) const {
    std::vector<Task> tasks;
    time_t current = last_generated_ > 0 ? last_generated_ : time(nullptr);

    if (last_generated_ == 0 && current <= up_to_timestamp) {
        tasks.push_back(base_task_);
        current += get_step();
    }

    while (current <= up_to_timestamp) {
        Task new_task = base_task_;
        tasks.push_back(new_task);
        current += get_step();
    }

    last_generated_ = up_to_timestamp;
    return tasks;
}

time_t TaskTemplate::get_step() const {
    switch (recurrence_type_) {
        case RecurrenceType::DAILY:
            return 86400; ///< 24*60*60
            break;
        case RecurrenceType::WEEKLY:
            return 604800; ///< 7*24*60*60
            break;
        case RecurrenceType::CUSTOM:
            return custom_interval_hours_ * 3600;
            break;
        default:
            return 0;
    }
}

///< Getters implementation
time_t TaskTemplate::get_last_generation_time() const noexcept {
    return last_generated_;
}

Task TaskTemplate::get_base_task() const noexcept {
    return base_task_;
}

TaskTemplate::RecurrenceType TaskTemplate::get_recurrence_type() const noexcept {
    return recurrence_type_;
}

int TaskTemplate::get_custom_interval_hours() const noexcept {
    return (recurrence_type_ == RecurrenceType::CUSTOM) ? custom_interval_hours_ : 0;
}