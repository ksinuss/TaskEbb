#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "task_template.hpp"
#include <chrono>

TEST_CASE("TaskTemplate Constructor Validation") {
    SUBCASE("Valid Custom Interval") {
        CHECK_NOTHROW(TaskTemplate("Valid Task", "Description", 5));
    }
    
    SUBCASE("Invalid Custom Interval") {
        CHECK_THROWS_AS(TaskTemplate("Invalid Task", "Description", 0), std::invalid_argument);
        CHECK_THROWS_AS(TaskTemplate("Invalid Task", "Description", -10), std::invalid_argument);
    }
}

TEST_CASE("Daily Recurrence Generation") {
    TaskTemplate test_template("Daily Meeting", "Standup", 24); // DAILY
    
    time_t now = time(nullptr);
    time_t future = now + 3 * 86400;
    
    auto tasks = test_template.generate_tasks(future);
    CHECK(tasks.size() == 3);
    
    SUBCASE("Task Properties") {
        CHECK(tasks[0].get_title() == "Daily Meeting");
        CHECK(tasks[1].get_description() == "Standup");
    }
}

TEST_CASE("Weekly Recurrence Generation") {
    TaskTemplate test_template("Weekly Report", "Summary", 168); // WEEKLY
    
    time_t future = time(nullptr) + 2 * 604800;
    auto tasks = test_template.generate_tasks(future);
    CHECK(tasks.size() == 2);
}

TEST_CASE("Custom Interval Generation") {
    TaskTemplate test_template("System Check", "Audit", 12); // 12 часов
    
    time_t future = time(nullptr) + 36 * 3600; // 3 интервала
    auto tasks = test_template.generate_tasks(future);
    CHECK(tasks.size() == 3);
}

TEST_CASE("Time Boundary Handling") {
    TaskTemplate test_template("Boundary Test", "Test", 24);
    
    SUBCASE("Zero Duration") {
        time_t now = time(nullptr);
        auto tasks = test_template.generate_tasks(now - 1);
        CHECK(tasks.empty());
    }
    
    SUBCASE("Exact Interval Match") {        
        time_t now = time(nullptr);
        time_t future = now + 86400; // 1 день
        auto tasks = test_template.generate_tasks(future);
        CHECK(tasks.size() == 1);
    }
}

TEST_CASE("Last Generated Time Tracking") {
    TaskTemplate test_template("Time Tracking", "Track", 24);
    
    time_t start = time(nullptr);
    time_t future = start + 5 * 86400;
    
    test_template.generate_tasks(future);
    CHECK(test_template.get_last_generation_time() == future);
    
    SUBCASE("Subsequent Generation") {
        time_t new_future = future + 86400;
        auto tasks = test_template.generate_tasks(new_future);
        CHECK(tasks.size() == 1);
    }
}

TEST_CASE("Getters Validation") {
    TaskTemplate test_template("Getters Test", "Check Getters", 100);
    
    CHECK(test_template.get_title() == "Getters Test");
    CHECK(test_template.get_recurrence_type() == TaskTemplate::RecurrenceType::CUSTOM);
    CHECK(test_template.get_interval_hours() == 100); 
}