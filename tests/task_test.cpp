#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "task.hpp"
#include <chrono>
#include <regex>

TEST_CASE("Testing Task class initialization") {
    SUBCASE("Default description") {
        Task task("Buy milk");
        
        CHECK(task.get_title() == "Buy milk");
        CHECK(task.get_description().empty());
        CHECK_FALSE(task.is_completed());
        CHECK(task.get_id().size() == 18); // 13 + _ + 4
    }

    SUBCASE("Custom description") {
        Task task("Write report", "Finish by Friday");
        
        CHECK(task.get_title() == "Write report");
        CHECK(task.get_description() == "Finish by Friday");
    }
}