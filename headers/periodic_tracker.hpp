#ifndef PERIODIC_TRACKER_HPP
#define PERIODIC_TRACKER_HPP

#include <chrono>
#include <stdexcept>
#include <optional>

/**
 * @class PeriodicTracker
 * @brief Records the first and second execution timestamps of a task, computes the interval between them, and predicts future execution dates
 */
class PeriodicTracker {
public:
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;

    /**
     * @brief Record a task execution event 
     * @param timestamp The time when the task was executed
     * @throws std::logic_error if called more than twice
     */
    void mark_execution(const TimePoint& timestamp);

    /**
     * @brief Calculate the interval between the first and second executions
     * @return Interval (in seconds)
     * @throws std::logic_error if less than two executions are recorded 
     */
    std::chrono::seconds get_interval() const;

    /**
     * @brief Get the next execution time (based on the calculated interval)
     * @return Time Point of the next execution or std::nullopt if interval is not set
     */
    std::optional<TimePoint> get_next_execution_time() const;

    /**
     * @brief Check if the interval has been calculated
     */
    bool is_interval_set() const noexcept;

    const std::optional<TimePoint>& get_first_execution() const noexcept;
    const std::optional<TimePoint>& get_second_execution() const noexcept;
    std::optional<TimePoint> get_last_execution() const noexcept;

private:
    std::optional<TimePoint> first_execution_;
    std::optional<TimePoint> second_execution_;
    std::optional<std::chrono::seconds> interval_;
};

#endif