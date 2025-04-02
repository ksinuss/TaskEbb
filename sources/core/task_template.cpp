#include "task_template.hpp"
#include <cmath>

TaskTemplate::TaskTemplate(const Task& base_task, RecurrenceType type, int interval)
    : base_task_(base_task), recurrence_type_(type), custom_interval_hours_(interval), last_generated_(0)
{
    if (recurrence_type_ == RecurrenceType::CUSTOM && interval <= 0) {
        throw std::invalid_argument("Custom interval must be positive");
    }
}