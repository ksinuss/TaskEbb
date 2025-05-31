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
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
        nullptr
    );
    
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db_);
        sqlite3_close_v2(db_);
        db_ = nullptr;
        throw std::runtime_error("Ошибка открытия БД: " + err);
    }
    
    try {
        initialize();
    } catch (...) {
        sqlite3_close_v2(db_);
        db_ = nullptr;
        throw;
    }
}

DatabaseManager::~DatabaseManager() {
    if (db_) {
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
}

void DatabaseManager::initialize() {
    try {
        executeQuery("PRAGMA foreign_keys = ON;");
        
        executeQuery(
            "CREATE TABLE IF NOT EXISTS tasks ("
            "id TEXT PRIMARY KEY, "
            "type INTEGER NOT NULL, "  // 0, 1 or 2
            "status INTEGER NOT NULL DEFAULT 0, "  // 0, 1 or 2
            "title TEXT NOT NULL, "
            "description TEXT, "
            "created_at INTEGER, "
            "deadline INTEGER, "
            "priority INTEGER, "
            "base_interval_seconds INTEGER, "
            "end_date INTEGER, "
            "last_execution INTEGER, "
            "next_execution INTEGER"
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
    } catch (const std::exception& e) {
        std::cerr << "Ошибка инициализации БД: " << e.what() << std::endl;
        throw;
    }
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

void DatabaseManager::throwOnError(int rc, const std::string& context) const {
    if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE) {
        std::ostringstream oss;
        oss << "SQLite error (" << context << "): " << sqlite3_errmsg(db_);
        throw std::runtime_error(oss.str());
    }
}

void DatabaseManager::executeQuery(const std::string& sql, const std::vector<std::string>& params) {
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare query");
    
    for (size_t i = 0; i < params.size(); ++i) {
        sqlite3_bind_text(stmt, i+1, params[i].c_str(), -1, SQLITE_TRANSIENT);
    }
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        throwOnError(rc, "execute query");
    }
    
    sqlite3_finalize(stmt);
}

void DatabaseManager::saveTask(const Task& task, const std::string& table_name) {
    const std::string sql = 
        "INSERT INTO " + table_name + " VALUES (?,?,?,?,?,?,?,?,?,?,?,?);";
    executeTaskQuery(sql, task);
}

void DatabaseManager::updateTask(const Task& task, const std::string& table_name) {
    const std::string sql = 
        "UPDATE " + table_name + " SET "
        "type=?,status=?,title=?,description=?,created_at=?,"
        "deadline=?,priority=?,base_interval_seconds=?,"
        "end_date=?,last_execution=?,next_execution=? "
        "WHERE id=?;";
    executeTaskQuery(sql, task);
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
    sqlite3_bind_int(stmt, 2, static_cast<int>(task.get_type()));
    sqlite3_bind_int(stmt, 3, static_cast<int>(task.get_status()));
    sqlite3_bind_text(stmt, 4, task.get_title().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, task.get_description().c_str(), -1, SQLITE_TRANSIENT);
    // created_at
    sqlite3_bind_int64(stmt, 6, QDateTime::currentSecsSinceEpoch());
    // deadline
    if (task.get_deadline().isValid()) {
        sqlite3_bind_int64(stmt, 7, task.get_deadline().toSecsSinceEpoch());
    } else {
        sqlite3_bind_null(stmt, 7);
    }
    // priority
    sqlite3_bind_null(stmt, 8);
    // interval
    sqlite3_bind_int(stmt, 9, task.get_interval().count() * 3600);
    // end_date
    if (task.get_end_date().isValid()) {
        sqlite3_bind_int64(stmt, 10, task.get_end_date().toSecsSinceEpoch());
    } else {
        sqlite3_bind_null(stmt, 10);
    }
    // last_execution
    auto last_exec = task.get_tracker().get_last_execution();
    if (last_exec) {
        sqlite3_bind_int64(stmt, 11, 
            std::chrono::system_clock::to_time_t(*last_exec));
    } else {
        sqlite3_bind_null(stmt, 11);
    }
    // next_execution (пока не используется)
    sqlite3_bind_null(stmt, 12);
}

Task DatabaseManager::mapTaskFromRow(sqlite3_stmt* stmt) {
    Task task;

    task.set_id(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    task.set_type(static_cast<Task::Type>(sqlite3_column_int(stmt, 1)));
    task.set_status(static_cast<Task::Status>(sqlite3_column_int(stmt, 2)));
    task.set_title(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
        task.set_description(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
    }
    // interval
    int seconds = sqlite3_column_int(stmt, 8);
    task.set_interval(std::chrono::hours(seconds / 3600));
    // end_date
    if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) {
        qint64 end_date = sqlite3_column_int64(stmt, 9);
        task.set_end_date(QDateTime::fromSecsSinceEpoch(end_date));
    }
    // last_execution
    if (sqlite3_column_type(stmt, 10) != SQLITE_NULL) {
        time_t last_exec = sqlite3_column_int64(stmt, 10);
        task.mark_execution(std::chrono::system_clock::from_time_t(last_exec));
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

std::vector<std::string> DatabaseManager::getAllChatIds() const {
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

std::vector<Task> DatabaseManager::getAllTasks(const std::string& table_name) {
    std::vector<Task> tasks;
    const std::string sql = "SELECT * FROM " + table_name + ";";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare SELECT all tasks");
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        tasks.push_back(mapTaskFromRow(stmt));
    }
    
    sqlite3_finalize(stmt);
    return tasks;
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

void DatabaseManager::executeTaskQuery(const std::string& sql, const Task& task) {
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare query");
    
    bindTaskParameters(stmt, task);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    throwOnError(rc, "execute query");
}

Task DatabaseManager::getTaskById(const std::string& id) {
    const std::string sql = "SELECT * FROM tasks WHERE id = ?;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    throwOnError(rc, "prepare SELECT by id");
    
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Task task = mapTaskFromRow(stmt);
        sqlite3_finalize(stmt);
        return task;
    }
    
    sqlite3_finalize(stmt);
    throw std::runtime_error("Task not found");
}

std::string DatabaseManager::getFirstChatId() const {
    auto chatIds = getAllChatIds();
    return chatIds.empty() ? "" : chatIds[0];
}
