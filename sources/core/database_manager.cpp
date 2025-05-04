#include "database_manager.hpp"
#include "task.hpp"
#include <sqlite3.h>
#include <stdexcept>
#include <sstream>

DatabaseManager::DatabaseManager(const std::string& db_path) : db_(nullptr) {
    int rc = sqlite3_open_v2(
        db_path.c_str(),
        &db_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        nullptr
    );
    throwOnError(rc, "open database");
    initialize();
}

DatabaseManager::~DatabaseManager() {
    if (db_) {
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
}

void DatabaseManager::initialize() {
    executeQuery(
        "CREATE TABLE IF NOT EXISTS tasks ("
        "id TEXT PRIMARY KEY, "
        "title TEXT NOT NULL, "
        "description TEXT, "
        "is_completed INTEGER DEFAULT 0, "
        "interval_hours INTEGER DEFAULT 0);"
    );

    executeQuery(
        "CREATE TABLE IF NOT EXISTS templates ("
        "id TEXT PRIMARY KEY, "
        "title TEXT NOT NULL, "
        "description TEXT, "
        "interval_hours INTEGER DEFAULT 0);"
    );

    executeQuery(
        "CREATE TABLE IF NOT EXISTS logs ("
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "action_type TEXT, "
        "task_id TEXT, "
        "message TEXT);"
    );
}

void DatabaseManager::throwOnError(int rc, const std::string& context) {
    if (rc != SQLITE_OK) {
        std::ostringstream oss;
        oss << "SQLite error (" << context << "): " << sqlite3_errmsg(db_);
        throw std::runtime_error(oss.str());
    }
}

void DatabaseManager::executeQuery(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string error = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("SQL error: " + error);
    }
}

void DatabaseManager::saveTask(const Task& task, const std::string& table_name, const std::function<void(sqlite3_stmt*, const Task&)>& bindParameters) {
    const std::string sql = 
        "INSERT INTO tasks (id, title, description, is_completed, interval_hours) "
        "VALUES (?, ?, ?, ?, ?);";
    
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare INSERT task");

    bindTaskParameters(stmt, task);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        throwOnError(rc, "execute INSERT task");
    }
    sqlite3_finalize(stmt);
}

std::vector<Task> DatabaseManager::getAllTasks(const std::string& table_name, const std::function<Task(sqlite3_stmt*)>& rowMapper) {
    std::vector<Task> tasks;
    const std::string sql = "SELECT * FROM tasks;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare SELECT tasks");
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        tasks.push_back(mapTaskFromRow(stmt));
    }
    if (rc != SQLITE_DONE) {
        throwOnError(rc, "fetch tasks");
    }
    sqlite3_finalize(stmt);
    return tasks;
}

void DatabaseManager::updateTask(const Task& task, const std::string& table_name, const std::function<void(sqlite3_stmt*, const Task&)>& bindParameters) {
    const std::string sql = 
        "UPDATE tasks SET "
        "title = ?, description = ?, is_completed = ?, interval_hours = ? "
        "WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare UPDATE task");

    sqlite3_bind_text(stmt, 1, task.get_title().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, task.get_description().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, task.is_completed() ? 1 : 0);
    sqlite3_bind_int(stmt, 4, task.get_interval().count());
    sqlite3_bind_text(stmt, 5, task.get_id().c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        throwOnError(rc, "execute UPDATE task");
    }
    sqlite3_finalize(stmt);
}

void DatabaseManager::deleteTask(const std::string& id, const std::string& table_name) {
    const std::string sql = "DELETE FROM tasks WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare DELETE task");
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        throwOnError(rc, "execute DELETE task");
    }
    sqlite3_finalize(stmt);
}

void DatabaseManager::logAction(const std::string& action_type, const std::string& task_id, const std::string& message) {
    const std::string sql = 
        "INSERT INTO logs (action_type, task_id, message) "
        "VALUES (?, ?, ?);";
    
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare INSERT log");

    sqlite3_bind_text(stmt, 1, action_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, task_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, message.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        throwOnError(rc, "execute INSERT log");
    }
    sqlite3_finalize(stmt);
}

void DatabaseManager::bindTaskParameters(sqlite3_stmt* stmt, const Task& task) {
    sqlite3_bind_text(stmt, 1, task.get_id().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, task.get_title().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, task.get_description().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, task.is_completed() ? 1 : 0);
    sqlite3_bind_int(stmt, 5, task.get_interval().count());
}

Task DatabaseManager::mapTaskFromRow(sqlite3_stmt* stmt) {
    Task task(
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)), // title
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))  // description
    );
    task.set_id(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))); // id
    task.mark_completed(sqlite3_column_int(stmt, 3) == 1);
    task.set_interval(std::chrono::hours(sqlite3_column_int(stmt, 4)));
    return task;
}

void DatabaseManager::saveTemplate(const TaskTemplate& tmpl) {
    const std::string sql = 
        "INSERT INTO templates (title, description, interval_hours) "
        "VALUES (?, ?, ?);";
    
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare INSERT template");

    sqlite3_bind_text(stmt, 1, tmpl.get_title().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, tmpl.get_description().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, tmpl.get_interval_hours());

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) throwOnError(rc, "execute INSERT template");
    sqlite3_finalize(stmt);
}

std::vector<TaskTemplate> DatabaseManager::getAllTemplates() {
    std::vector<TaskTemplate> templates;
    const std::string sql = "SELECT title, description, interval_hours FROM templates;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare SELECT templates");
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        int interval = sqlite3_column_int(stmt, 2);
        templates.emplace_back(title, desc, interval);
    }
    
    sqlite3_finalize(stmt);
    return templates;
}

std::pair<int, int> DatabaseManager::getTaskStats() {
    int completed = 0, pending = 0;
    const std::string sql_completed = "SELECT COUNT(*) FROM tasks WHERE is_completed = 1";
    const std::string sql_pending = "SELECT COUNT(*) FROM tasks WHERE is_completed = 0";

    sqlite3_stmt* stmt;
    
    sqlite3_prepare_v2(db_, sql_completed.c_str(), -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        completed = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db_, sql_pending.c_str(), -1, &stmt, nullptr);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        pending = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    return {completed, pending};
}