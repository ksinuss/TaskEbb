#include "database_manager.hpp"
#include "task.hpp"
#include <sqlite3.h>
#include <stdexcept>
#include <sstream>
#include <iostream>

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
        "first_execution INTEGER, "
        "second_execution INTEGER, "
        "is_recurring BOOLEAN DEFAULT FALSE, "
        "type INTEGER, "
        "deadline INTEGER NULL, "
        "interval_hours INTEGER, "
        "end_date INTEGER"
        ");"
    );

    executeQuery(
        "CREATE TABLE IF NOT EXISTS telegram_chats ("
        "chat_id TEXT PRIMARY KEY"
        ");"
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

    addColumnIfNotExists("tasks", "first_execution", "INTEGER");
    addColumnIfNotExists("tasks", "second_execution", "INTEGER");
    addColumnIfNotExists("tasks", "is_recurring", "BOOLEAN DEFAULT FALSE");
    addColumnIfNotExists("tasks", "type", "INTEGER");
    addColumnIfNotExists("tasks", "deadline", "INTEGER NULL");
    addColumnIfNotExists("tasks", "interval_hours", "INTEGER");
    addColumnIfNotExists("tasks", "end_date", "INTEGER");
}

void DatabaseManager::addColumnIfNotExists(const std::string& table, const std::string& column, const std::string& type) {
    std::string checkSql = 
        "SELECT COUNT(*) FROM pragma_table_info('" + table + "') "
        "WHERE name = '" + column + "'";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, checkSql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Failed to check column existence");
    }

    bool columnExists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        columnExists = (sqlite3_column_int(stmt, 0) > 0);
    }
    sqlite3_finalize(stmt);

    if (!columnExists) {
        std::string alterSql = 
            "ALTER TABLE " + table + 
            " ADD COLUMN " + column + " " + type;
        executeQuery(alterSql);
    }
}

void DatabaseManager::throwOnError(int rc, const std::string& context) {
    if (rc != SQLITE_OK) {
        std::ostringstream oss;
        oss << "SQLite error (" << context << "): " << sqlite3_errmsg(db_);
        throw std::runtime_error(oss.str());
    }
}

void DatabaseManager::executeQuery(const std::string& sql) {
    std::cout << "[SQL] Executing: " << sql << std::endl;
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[SQL ERROR] " << errMsg << " (query: " << sql << ")" << std::endl;
        sqlite3_free(errMsg);
        throw std::runtime_error("SQL error");
    }
}

void DatabaseManager::saveTask(const Task& task, const std::string& table_name, const std::function<void(sqlite3_stmt*, const Task&)>& bindParameters) {
    const std::string sql = 
        "INSERT INTO tasks (id, title, description, is_completed, first_execution, second_execution, is_recurring, type, deadline, interval_hours, end_date) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    
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
    if (!tableExists("tasks")) {
        throw std::runtime_error("Таблица 'tasks' не существует.");
    }
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
        "title = ?, description = ?, is_completed = ?, "
        "first_execution = ?, second_execution = ?, "
        "is_recurring = ?, type = ?, deadline = ?, interval_hours = ?, end_date = ? "
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
    sqlite3_bind_text(stmt, 1, task.get_id().c_str(), -1, SQLITE_TRANSIENT); ///< id
    sqlite3_bind_text(stmt, 2, task.get_title().c_str(), -1, SQLITE_TRANSIENT); ///< title
    sqlite3_bind_text(stmt, 3, task.get_description().c_str(), -1, SQLITE_TRANSIENT); ///< description
    sqlite3_bind_int(stmt, 4, task.is_completed() ? 1 : 0); ///< is_completed

    auto first_exec = task.get_tracker().get_first_execution();
    auto second_exec = task.get_tracker().get_second_execution();
    
    sqlite3_bind_int64(stmt, 5, first_exec.has_value() ? std::chrono::system_clock::to_time_t(*first_exec) : 0);
    sqlite3_bind_int64(stmt, 6, second_exec.has_value() ? std::chrono::system_clock::to_time_t(*second_exec) : 0);

    sqlite3_bind_int(stmt, 7, task.is_recurring() ? 1 : 0); ///< is_recurring
    sqlite3_bind_int(stmt, 8, static_cast<int>(task.get_type())); ///< type
    if (task.get_type() == Task::Type::Deadline && task.get_deadline().isValid()) {
        sqlite3_bind_int64(stmt, 9, task.get_deadline().toSecsSinceEpoch());
    } else {
        sqlite3_bind_null(stmt, 9);
    }
    sqlite3_bind_int(stmt, 10, task.get_interval().count()); ///< interval_hours
    if (task.get_end_date().isValid()) {
        sqlite3_bind_int64(stmt, 11, task.get_end_date().toSecsSinceEpoch());
    } else {
        sqlite3_bind_null(stmt, 11);
    } ///< end_date
}

Task DatabaseManager::mapTaskFromRow(sqlite3_stmt* stmt) {
    Task task(
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)), // title (индекс 1)
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))  // description (индекс 2)
    );

    task.set_id(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))); // id (индекс 0)
    task.mark_completed(sqlite3_column_int(stmt, 3) == 1); // is_completed (индекс 3)

    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
        time_t first_exec = sqlite3_column_int64(stmt, 4);
        task.mark_execution(std::chrono::system_clock::from_time_t(first_exec));
    }

    if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
        time_t second_exec = sqlite3_column_int64(stmt, 5);
        task.mark_execution(std::chrono::system_clock::from_time_t(second_exec));
    }

    task.set_recurring(sqlite3_column_int(stmt, 6) == 1); // is_recurring (индекс 6)
    Task::Type type = static_cast<Task::Type>(sqlite3_column_int(stmt, 7));

    if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
        qint64 deadlineSecs = sqlite3_column_int64(stmt, 8);
        if (deadlineSecs > 0) {
            task.set_deadline(QDateTime::fromSecsSinceEpoch(deadlineSecs));
        } else {
            task.set_deadline(QDateTime());
        }
    } else {
        task.set_deadline(QDateTime());
    }

    task.set_interval(std::chrono::hours(sqlite3_column_int(stmt, 9))); // interval_hours (индекс 9)

    if (sqlite3_column_type(stmt, 10) != SQLITE_NULL) {
        task.set_end_date(QDateTime::fromSecsSinceEpoch(sqlite3_column_int64(stmt, 10)));
    }

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

void DatabaseManager::saveChatId(const std::string& chat_id) {
    try {
        executeQuery("CREATE TABLE IF NOT EXISTS telegram_chats (chat_id TEXT PRIMARY KEY);");
        
        const std::string sql = "INSERT OR IGNORE INTO telegram_chats (chat_id) VALUES (?);";
        sqlite3_stmt* stmt = nullptr;
        
        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("Ошибка подготовки запроса: " + std::string(sqlite3_errmsg(db_)));
        }
        sqlite3_bind_text(stmt, 1, chat_id.c_str(), -1, SQLITE_TRANSIENT);
        rc = sqlite3_step(stmt);
        
        sqlite3_finalize(stmt);
        
        if (rc != SQLITE_DONE) {
            if (rc == SQLITE_CONSTRAINT) {
                std::cout << "[INFO] Chat ID " << chat_id << " уже существует." << std::endl;
            } else {
                throw std::runtime_error("Ошибка выполнения запроса: " + std::string(sqlite3_errmsg(db_)));
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] DatabaseManager::saveChatId: " << e.what() << std::endl;
        throw;
    }
}

std::vector<std::string> DatabaseManager::getAllChatIds() {
    std::vector<std::string> chat_ids;
    const std::string sql = "SELECT chat_id FROM telegram_chats;";
    sqlite3_stmt* stmt = nullptr;
    
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare SELECT chat_ids");
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        chat_ids.push_back(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))
        );
    }
    sqlite3_finalize(stmt);
    return chat_ids;
}

void DatabaseManager::unlinkAllAccounts() {
    executeQuery("DELETE FROM telegram_chats;");
}

void DatabaseManager::deleteAllChatIds() {
    executeQuery("DELETE FROM telegram_chats;");
}

bool DatabaseManager::tableExists(const std::string& tableName) {
    std::string sql = "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Ошибка проверки существования таблицы.");
    }
    sqlite3_bind_text(stmt, 1, tableName.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    bool exists = (rc == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0);
    sqlite3_finalize(stmt);
    return exists;
}
