#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "task_template.hpp"
#include <chrono>

TEST_CASE("TaskTemplate Constructor Validation") {
    Task base_task("Valid Task");
    
    SUBCASE("Valid Custom Interval") {
        CHECK_NOTHROW(TaskTemplate(base_task, TaskTemplate::RecurrenceType::CUSTOM, 5));
    }
    
    SUBCASE("Invalid Custom Interval") {
        CHECK_THROWS_AS(TaskTemplate(base_task, TaskTemplate::RecurrenceType::CUSTOM, 0), std::invalid_argument);
        CHECK_THROWS_AS(TaskTemplate(base_task, TaskTemplate::RecurrenceType::CUSTOM, -10), std::invalid_argument);
    }
}

TEST_CASE("Daily Recurrence Generation") {
    Task base("Daily Meeting");
    TaskTemplate test_template(base, TaskTemplate::RecurrenceType::DAILY);
    
    time_t now = time(nullptr);
    time_t future = now + 3 * 86400;
    
    auto tasks = test_template.generate_tasks(future);
    CHECK(tasks.size() == 3);
    
    SUBCASE("Task Properties") {
        CHECK(tasks[0].get_title() == "Daily Meeting");
        CHECK(tasks[1].get_description() == base.get_description());
    }
}

TEST_CASE("Weekly Recurrence Generation") {
    Task base("Weekly Report");
    TaskTemplate test_template(base, TaskTemplate::RecurrenceType::WEEKLY);
    
    time_t future = time(nullptr) + 2 * 604800;
    
    auto tasks = test_template.generate_tasks(future);
    CHECK(tasks.size() == 2);
}

TEST_CASE("Custom Interval Generation") {
    Task base("System Check");
    TaskTemplate test_template(base, TaskTemplate::RecurrenceType::CUSTOM, 12); ///< Every 12 hours
    
    time_t future = time(nullptr) + 36 * 3600; ///< 3 intervals
    
    auto tasks = test_template.generate_tasks(future);
    CHECK(tasks.size() == 3);
}

TEST_CASE("Time Boundary Handling") {
    Task base("Boundary Test");
    TaskTemplate test_template(base, TaskTemplate::RecurrenceType::DAILY);
    
    SUBCASE("Zero Duration") {
        time_t now = time(nullptr);
        auto tasks = test_template.generate_tasks(now - 1); ///< Request to past
        CHECK(tasks.empty());
    }
    
    SUBCASE("Exact Interval Match") {        
        time_t now = time(nullptr);
        time_t future = now + 86400; ///< 1 day
        
        auto tasks = test_template.generate_tasks(future);
        CHECK(tasks.size() == 1);
    }
}

TEST_CASE("Last Generated Time Tracking") {
    Task base("Time Tracking");
    TaskTemplate test_template(base, TaskTemplate::RecurrenceType::DAILY);
    
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
    Task base("Getters Test", "Description");
    TaskTemplate test_template(base, TaskTemplate::RecurrenceType::WEEKLY, 100);
    
    CHECK(test_template.get_base_task().get_title() == "Getters Test");
    CHECK(test_template.get_recurrence_type() == TaskTemplate::RecurrenceType::WEEKLY);
    CHECK(test_template.get_custom_interval_hours() == 0); 
}