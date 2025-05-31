#include "task.hpp"
#include <chrono>
#include <random>
#include <cstdio>
#include <stdexcept>
#include <cstring>

Task::Task() : type_(OneTime), status_(Active), is_completed_(false), interval_(0), is_recurring_(false) {
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    std::snprintf(id_, sizeof(id_), "TEMP_%lld", static_cast<long long>(millis));
}

Task::Task(const std::string& title, const std::string& description)
    : title_(title),
      description_(description),
      type_(Type::OneTime),
      status_(Active),
      is_completed_(false),
      interval_(0),
      is_recurring_(false)
{
    auto now = std::chrono::system_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::snprintf(id_, sizeof(id_), "TEMP_%lld", static_cast<long long>(millis));
}

Task::Task(const std::string& title, const std::string& description, Type type, const QDateTime& deadline, std::chrono::hours interval, const QDateTime& endDate)
    : title_(title),
      description_(description),
      type_(type),
      deadline_(deadline),
      interval_(interval),
      endDate_(endDate),
      is_completed_(false),
      tracker_(),
      is_recurring_(type == Recurring)
{
    if (title.empty()) {
        throw std::invalid_argument("Заголовок задачи не может быть пустым.");
    }

    if (type_ == Type::Deadline) {
        if (!deadline_.isValid()) {
            throw std::invalid_argument("Некорректный дедлайн.");
        }
    }

    if (type_ == Type::Recurring) {
        if (interval_.count() <= 0) {
            throw std::invalid_argument("Интервал для периодической задачи должен быть положительным.");
        }
        is_recurring_ = true;
    }

    if (endDate_.isValid() && endDate_ <= QDateTime::currentDateTime()) {
        throw std::invalid_argument("Дата окончания должна быть в будущем.");
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

void Task::set_deadline(const QDateTime& deadline) {
    if (type_ == Type::Deadline) {
        if (deadline.isValid() && deadline >= QDateTime::currentDateTime()) {
            deadline_ = deadline;
        } else {
            throw std::invalid_argument("Некорректный дедлайн");
        }
    } else {
        deadline_ = QDateTime();
    }
}

void Task::set_end_date(const QDateTime& endDate) {
    endDate_ = endDate;
}

Task::Type Task::get_type() const noexcept {
    return type_;
}

Task::Status Task::get_status() const noexcept {
    return status_;
}

QDateTime Task::get_deadline() const noexcept {
    return deadline_;
}

QDateTime Task::get_end_date() const noexcept {
    return endDate_;
}

void Task::set_type(Type type) {
    if (type < Type::OneTime || type > Type::Recurring) {
        throw std::invalid_argument("Invalid task type");
    }
    type_ = type;
}

void Task::set_status(Status status) {
    if (status < Status::Active || status > Status::Archived) {
        throw std::invalid_argument("Invalid task status");
    }
    status_ = status;
}

bool Task::is_valid() const {
    switch (type_) {
        case Deadline:
            return deadline_.isValid() && deadline_ > QDateTime::currentDateTime();
        case Recurring:
            if (interval_.count() <= 0) return false;
            if (endDate_.isValid() && endDate_ < QDateTime::currentDateTime()) {
                return false;
            }
            return true;
        default: ///< OneTime
            return true;
    }
}

QString Task::validation_error() const {
    switch (type_) {
        case Deadline:
            if (!deadline_.isValid()) 
                return "Некорректная дата/время дедлайна";
            if (deadline_ < QDateTime::currentDateTime()) 
                return "Дедлайн не может быть в прошлом";
            return "";
        case Recurring:
            if (interval_.count() <= 0) 
                return "Интервал должен быть положительным";
            if (endDate_.isValid() && endDate_ < QDateTime::currentDateTime()) 
                return "Дата окончания не может быть в прошлом";
            if (endDate_.isValid() && 
                QDateTime::currentDateTime().daysTo(endDate_) < interval_.count() / 24) 
            {
                return "Период должен быть больше интервала повторений";
            }
            return "";
        default:
            return "";
    }
}
