#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "task.hpp"
#include <chrono>
#include <regex>

TEST_CASE("ID generation and validation") {
    Task task1(
        "Task 1", 
        "", 
        Task::Type::OneTime,
        QDateTime(),
        std::chrono::hours(0),
        QDateTime()
    );
    Task task2(
        "Task 2",
        "Описание",
        Task::Type::Deadline,
        QDateTime::currentDateTime().addDays(1),
        std::chrono::hours(0),
        QDateTime()
    );

    CHECK(task1.get_id().size() == 18);
    CHECK(std::regex_match(task1.get_id(), std::regex(R"(\d{13}_\d{4})")));
    
    CHECK(task1.get_id() != task2.get_id());
}

TEST_CASE("Empty title throws exception") {
    CHECK_THROWS_AS(
        Task("", "", Task::Type::OneTime),
        std::invalid_argument
    );
}

TEST_CASE("Getters return correct values") {
    Task task("Buy milk", "Urgent");

    CHECK(task.get_title() == "Buy milk");
    CHECK(task.get_description() == "Urgent");
    CHECK(!task.is_completed());
}

TEST_CASE("Setting interval works correctly") {
    Task task("Learn C++", "");
    task.set_interval(std::chrono::hours(72));

    CHECK(task.get_interval() == std::chrono::hours(72));
}

TEST_CASE("Marking task as completed") {
    Task task("Read a book", "");
    task.mark_completed(true);

    CHECK(task.is_completed());
}