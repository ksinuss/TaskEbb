#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "periodic_tracker.hpp"
#include <chrono>

using namespace std::chrono_literals;

TEST_CASE("No interval after one execution") {
    PeriodicTracker tracker;
    auto time = PeriodicTracker::Clock::now();
    tracker.mark_execution(time);
    
    CHECK(!tracker.is_interval_set());
    CHECK(tracker.get_next_execution_time() == std::nullopt);
}

TEST_CASE("Correct interval calculation") {
    PeriodicTracker tracker;
    auto t1 = PeriodicTracker::Clock::now();
    auto t2 = t1 + 2h;
    
    tracker.mark_execution(t1);
    tracker.mark_execution(t2);
    
    CHECK(tracker.get_interval() == 2h);
    CHECK(tracker.get_next_execution_time().value() == t2 + 2h);
}

TEST_CASE("Third execution throws exception") {
    PeriodicTracker tracker;
    auto t1 = PeriodicTracker::Clock::now();
    
    tracker.mark_execution(t1);
    tracker.mark_execution(t1 + 1h);
    CHECK_THROWS_AS(tracker.mark_execution(t1 + 3h), std::logic_error);
}

TEST_CASE("Get interval before two executions throws") {
    PeriodicTracker tracker;
    auto t1 = PeriodicTracker::Clock::now();
    tracker.mark_execution(t1);
    
    CHECK_THROWS_AS(tracker.get_interval(), std::logic_error);
}

TEST_CASE("Zero interval handling") {
    PeriodicTracker tracker;
    auto t1 = PeriodicTracker::Clock::now();
    
    tracker.mark_execution(t1);
    tracker.mark_execution(t1);
    
    CHECK(tracker.get_interval() == 0s);
    CHECK(tracker.get_next_execution_time().value() == t1);
}

TEST_CASE("Negative interval (impossible in practice)") {
    PeriodicTracker tracker;
    auto t1 = PeriodicTracker::Clock::now();
    auto t2 = t1 - 1h;
    
    tracker.mark_execution(t1);
    tracker.mark_execution(t2);
    
    CHECK(tracker.get_interval() == -1h);
    CHECK(tracker.get_next_execution_time().value() == t2 + (-1h));
}

TEST_CASE("is_interval_set correctness") {
    PeriodicTracker tracker;
    auto t1 = PeriodicTracker::Clock::now();
    
    CHECK_FALSE(tracker.is_interval_set());
    tracker.mark_execution(t1);
    CHECK_FALSE(tracker.is_interval_set());
    tracker.mark_execution(t1 + 5min);
    CHECK(tracker.is_interval_set());
}