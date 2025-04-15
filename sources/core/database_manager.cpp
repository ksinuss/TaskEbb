#include "database_manager.hpp"
#include "task.hpp"
#include <sqlite3.h>
#include <stdexcept>
#include <sstream>

DatabaseManager::~DatabaseManager() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

DatabaseManager::DatabaseManager(const std::string& db_path) {
    int rc = sqlite3_open_v2(
        db_path.c_str(),
        &db_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        nullptr
    );
    throwOnError(rc, "open database");

    createTable(
        "CREATE TABLE IF NOT EXISTS tasks ("
        "id TEXT PRIMARY KEY, "
        "title TEXT NOT NULL, "
        "description TEXT, "
        "is_completed INTEGER, "
        "interval_hours INTEGER)"
    );

    createTable(
        "CREATE TABLE IF NOT EXISTS templates ("
        "base_task_id TEXT, "
        "recurrence_type INTEGER, "
        "custom_interval_hours INTEGER)"
    );
}

void DatabaseManager::throwOnError(int rc, const std::string& context) {
    if (rc != SQLITE_OK) {
        std::ostringstream oss;
        oss << "SQLite error (" << context << "): " << sqlite3_errmsg(db_);
        throw std::runtime_error(oss.str());
    }
}

void DatabaseManager::createTable(const std::string& create_sql) {
    executeQuery(create_sql);
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

template <typename T>
void DatabaseManager::saveTask(
    const T& task,
    const std::string& table_name,
    const std::function<void(sqlite3_stmt*, const T&)>& bindParameters
) {
    std::string sql = "INSERT INTO " + table_name + " VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare insert");

    bindParameters(stmt, task);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throwOnError(rc, "execute insert");
    }

    sqlite3_finalize(stmt);
}

template <typename T>
std::vector<T> DatabaseManager::getAllTasks(
    const std::string& table_name,
    const std::function<T(sqlite3_stmt*)>& rowMapper
) {
    std::vector<T> result;
    std::string sql = "SELECT * FROM " + table_name;
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare select");

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        result.push_back(rowMapper(stmt));
    }

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throwOnError(rc, "fetch data");
    }

    sqlite3_finalize(stmt);
    return result;
}

template <typename T>
void DatabaseManager::updateTask(
    const T& task,
    const std::string& table_name,
    const std::function<void(sqlite3_stmt*, const T&)>& bindParameters
) {
    std::string sql = 
        "UPDATE " + table_name + " SET "
        "title = ?, description = ?, is_completed = ?, interval_hours = ? "
        "WHERE id = ?";
    
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare update");

    bindParameters(stmt, task);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throwOnError(rc, "execute update");
    }

    sqlite3_finalize(stmt);
}

void DatabaseManager::deleteTask(const std::string& id, const std::string& table_name) {
    std::string sql = "DELETE FROM " + table_name + " WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare delete");

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throwOnError(rc, "execute delete");
    }

    sqlite3_finalize(stmt);
}

template void DatabaseManager::saveTask<Task>(
    const Task&,
    const std::string&,
    const std::function<void(sqlite3_stmt*, const Task&)>&
);

template std::vector<Task> DatabaseManager::getAllTasks<Task>(
    const std::string&,
    const std::function<Task(sqlite3_stmt*)>&
);

template void DatabaseManager::updateTask<Task>(
    const Task&,
    const std::string&,
    const std::function<void(sqlite3_stmt*, const Task&)>&
);