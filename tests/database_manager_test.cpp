#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "database_manager.hpp"
#include "task.hpp"
#include <chrono>
#include <filesystem>
#include <functional>

using namespace std::chrono_literals;
namespace fs = std::filesystem;

Task create_test_task() {
    Task task("Test Title", "Test Description");
    task.set_interval(24h);
    task.mark_completed(true);
    return task;
}

// Deleting the test database before each run
struct DatabaseTestFixture {
    const std::string TEST_DB_PATH = "test_db.sqlite";
    DatabaseTestFixture() {
        if (fs::exists(TEST_DB_PATH)) {
            fs::remove(TEST_DB_PATH);
        }
    }
};

// Parameter binding function for Task objects
std::function<void(sqlite3_stmt*, const Task&)> bindTask = [](sqlite3_stmt* stmt, const Task& t) {
    sqlite3_bind_text(stmt, 1, t.get_id().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, t.get_title().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, t.get_description().c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, t.is_completed() ? 1 : 0);
    sqlite3_bind_int(stmt, 5, static_cast<int>(t.get_interval().count()));
};

// Row-to-object mapping function for Task
std::function<Task(sqlite3_stmt*)> rowMapper = [](sqlite3_stmt* stmt) {
    return Task(
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)), // title
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)) //description
    );
};

TEST_CASE_FIXTURE(DatabaseTestFixture, "Save and retrieve task") {
    DatabaseManager db(TEST_DB_PATH);
    Task task = create_test_task();
    
    CHECK_NOTHROW(db.saveTask<Task>(task, "tasks", bindTask));
    
    auto tasks = db.getAllTasks<Task>("tasks", rowMapper);
    CHECK(tasks.size() == 1);
    CHECK(tasks[0].get_title() == "Test Title");
}

TEST_CASE_FIXTURE(DatabaseTestFixture, "Update task") {
    DatabaseManager db(TEST_DB_PATH);
    Task task = create_test_task();
    
    db.saveTask<Task>(task, "tasks", bindTask);
    task.mark_completed(false);
    db.updateTask<Task>(task, "tasks", bindTask);
    
    auto tasks = db.getAllTasks<Task>("tasks", rowMapper);
    CHECK(tasks[0].is_completed() == false);
}

TEST_CASE_FIXTURE(DatabaseTestFixture, "Delete task") {
    DatabaseManager db(TEST_DB_PATH);
    Task task = create_test_task();
    
    db.saveTask<Task>(task, "tasks", bindTask);
    db.deleteTask(task.get_id(), "tasks");
    
    auto tasks = db.getAllTasks<Task>("tasks", rowMapper);
    CHECK(tasks.empty());
}

TEST_CASE_FIXTURE(DatabaseTestFixture, "Invalid operations") {
    DatabaseManager db(TEST_DB_PATH);
    
    // Attempt to save to a non-existent table
    CHECK_THROWS(db.saveTask<Task>(create_test_task(), "unknown_table", bindTask));
    
    // Attempt to delete a non-existent task
    CHECK_NOTHROW(db.deleteTask("dummy_id", "tasks"));
}