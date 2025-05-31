#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "database_manager.hpp"
#include "task.hpp"
#include <chrono>
#include <filesystem>
#include <functional>

using namespace std::chrono_literals;
namespace fs = std::filesystem;

TEST_CASE("Save and retrieve task") {
    DatabaseManager db(":memory:");
    Task task("Test task", "Description", Task::Type::OneTime);
    
    CHECK_NOTHROW(db.saveTask(task, "tasks"));
    
    auto tasks = db.getAllTasks("tasks");
    REQUIRE(tasks.size() == 1);
    CHECK(tasks[0].get_title() == "Test task");
}

TEST_CASE("Update task") {
    DatabaseManager db(":memory:");
    Task task("Original", "Desc", Task::Type::OneTime);
    
    db.saveTask(task, "tasks");
    
    task.set_title("Updated");
    
    CHECK_NOTHROW(db.updateTask(task, "tasks"));
    
    auto tasks = db.getAllTasks("tasks");
    REQUIRE(tasks.size() == 1);
    CHECK(tasks[0].get_title() == "Updated");
}

TEST_CASE("Delete task") {
    DatabaseManager db(":memory:");
    Task task("To delete", "", Task::Type::OneTime);
    
    db.saveTask(task, "tasks");
    
    auto tasks_before = db.getAllTasks("tasks");
    REQUIRE(tasks_before.size() == 1);
    
    db.deleteTask(task.get_id(), "tasks");
    
    auto tasks_after = db.getAllTasks("tasks");
    CHECK(tasks_after.empty());
}

TEST_CASE("Invalid table name") {
    DatabaseManager db(":memory:");
    Task task("Test", "Should fail");
    
    CHECK_THROWS(db.saveTask(task, "invalid_table"));
}