#include "task.hpp"
#include <chrono>
#include <random>
#include <cstdio>
#include <stdexcept>
#include <cstring>

Task::Task(const std::string& title, const std::string& description)
    : title_(title), description_(description), is_completed_(false), interval_(std::chrono::hours(0))
{
    // Check for empty title
    if (title.empty()) {
        throw std::invalid_argument("Title cannot be empty");
    }

    ///< Generate id
    // Get current time (in mlsec)
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto datetime = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    // Generate random num
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dist(0, 9999);

    // Fixed-length id
    std::snprintf(
        id_, sizeof(id_), 
        "%013lld_%04d", 
        static_cast<long long>(datetime), static_cast<int>(dist(gen))
    ); 
}

std::string Task::get_id() const {
    return std::string(id_);
}

std::string Task::get_title() const {
    return title_;
}

std::string Task::get_description() const {
    return description_;
}

std::chrono::hours Task::get_interval() const noexcept{ 
    return interval_; 
}

bool Task::is_completed() const {
    return is_completed_;
} 

void Task::set_interval(std::chrono::hours interval) {
    this->interval_ = interval;
}

void Task::mark_completed(bool status) {
    this->is_completed_ = status;
}

void Task::set_id(const std::string& id) {
    size_t len = id.size() < 18 ? id.size() : 18;
    strncpy(id_, id.c_str(), len);
    id_[len] = '\0';
}

void Task::set_title(const std::string& title) {
    if (title.empty()) {
        throw std::invalid_argument("Заголовок задачи не может быть пустым.");
    }
    title_ = title;
}

void Task::set_description(const std::string& description) {
    description_ = description;
}

void Task::mark_execution(const PeriodicTracker::TimePoint& timestamp) {
    if (is_recurring_) {
        tracker_.mark_execution(timestamp);
    } else {
        mark_completed(true);
    }
}

std::chrono::hours Task::get_calculated_interval() const {
    if (!tracker_.is_interval_set()) {
        throw std::logic_error("Интервал не рассчитан");
    }
    return duration_cast<std::chrono::hours>(tracker_.get_interval());
}

const PeriodicTracker& Task::get_tracker() const noexcept {
    return tracker_;
}

void Task::set_recurring(bool status) noexcept { 
    is_recurring_ = status; 
}

bool Task::is_recurring() const noexcept { 
    return is_recurring_; 
}
