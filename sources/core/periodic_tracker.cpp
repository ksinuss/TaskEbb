#include "periodic_tracker.hpp"
#include <stdexcept>

void PeriodicTracker::mark_execution(const PeriodicTracker::TimePoint& timestamp) {
    if (first_execution_.has_value() && second_execution_.has_value()) {
        throw std::logic_error("Maximum of two executions can be recorded");
    }

    if (!first_execution_.has_value()) {
        first_execution_ = timestamp;
    } else {
        second_execution_ = timestamp;
        ///< Calculate interval (when both executions are recorded)
        interval_ = std::chrono::duration_cast<std::chrono::seconds>(*second_execution_ - *first_execution_);
    }
}

std::chrono::seconds PeriodicTracker::get_interval() const {
    if (!interval_.has_value()) {
        throw std::logic_error("Interval is not calculated yet");
    }
    return *interval_;
}

std::optional<PeriodicTracker::TimePoint> PeriodicTracker::get_next_execution_time() const {
    if (!interval_.has_value() || !second_execution_.has_value()) {
        return std::nullopt;
    }
    return *second_execution_ + *interval_;
}

bool PeriodicTracker::is_interval_set() const noexcept {
    return interval_.has_value();
}

const std::optional<PeriodicTracker::TimePoint>& PeriodicTracker::get_first_execution() const noexcept {
    return first_execution_;
}

const std::optional<PeriodicTracker::TimePoint>& PeriodicTracker::get_second_execution() const noexcept {
    return second_execution_;
}

std::optional<PeriodicTracker::TimePoint> PeriodicTracker::get_last_execution() const noexcept {
    if (second_execution_.has_value()) {
        return second_execution_;
    }
    return first_execution_;
}