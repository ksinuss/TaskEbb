#include "task.hpp"
#include <chrono>
#include <random>
#include <cstdio>
#include <stdexcept>

Task::Task(const std::string& title, const std::string& description)
    : title_(title), description_(description), is_completed_(false), interval_(std::chrono::hours(0))
{
    // check for empty title
    if (title.empty()) {
        throw std::invalid_argument("Title cannot be empty");
    }

    ///< generate id
    // get current time (in mlsec)
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto datetime = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    // generate random num
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dist(0, 9999);

    // fixed-length id
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

std::chrono::hours Task::get_interval() const { 
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