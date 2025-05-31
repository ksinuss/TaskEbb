#include "task_template.hpp"
#include "task.hpp"
#include <cmath>

TaskTemplate::TaskTemplate(const std::string& title, const std::string& description, TemplateType type, int interval)
    : base_task_(title, description, static_cast<Task::Type>(type)),
      template_type_(type),
      recurrence_type_(RecurrenceType::CUSTOM),
      custom_interval_hours_(interval), 
      last_generated_(0)  
{}

TaskTemplate::TaskTemplate(const std::string& title, const std::string& description, int interval_hours)
    : base_task_(title, description, Task::Type::Recurring),
      template_type_(TemplateType::PERIODIC),
      recurrence_type_(RecurrenceType::CUSTOM),
      custom_interval_hours_(interval_hours),
      last_generated_(0) 
{}

std::vector<Task> TaskTemplate::generate_tasks(time_t up_to_timestamp) const {
    std::vector<Task> tasks;
    time_t current = last_generated_ > 0 ? last_generated_ : time(nullptr);

    if (last_generated_ == 0 && current < up_to_timestamp) {
        tasks.push_back(base_task_);
        current += get_step();
    }

    while (current < up_to_timestamp) {
        tasks.push_back(base_task_);
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

std::string TaskTemplate::get_title() const { 
    return base_task_.get_title(); 
}

std::string TaskTemplate::get_description() const { 
    return base_task_.get_description(); 
}
    
int TaskTemplate::get_interval_hours() const { 
    return custom_interval_hours_; 
}