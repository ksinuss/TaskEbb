#ifndef DATABASE_MANAGER_HPP
#define DATABASE_MANAGER_HPP

#include <sqlite3.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <functional>
#include <utility>
#include "task.hpp"
#include "task_template.hpp"

/**
 * @class DatabaseManager
 * @brief Manages SQLite database operations including table creation, and CRUD operations for tasks and templates.
 */
class DatabaseManager {
public:
    /**
     * @brief Initializes the database connection and creates required tables.
     * @param db_path Path to the SQLite database file.
     * @throws std::runtime_error If connection fails.
     */
    explicit DatabaseManager(const std::string& db_path);

    /**
     * @brief Destroy the Database Manager object
     */
    ~DatabaseManager();

    void initialize();

    void logAction(
        const std::string& action_type,
        const std::string& task_id,
        const std::string& message
    );

    void saveTask(const Task& task, const std::string& table_name = "tasks");
    void updateTask(const Task& task, const std::string& table_name = "tasks");
    void deleteTask(const std::string& id, const std::string& table_name = "tasks");
    Task getTaskById(const std::string& id);
    std::vector<Task> getAllTasks(const std::string& table_name = "tasks");

    void saveTemplate(const TaskTemplate& tmpl);
    void deleteTemplate(const std::string& id);
    std::vector<TaskTemplate> getAllTemplates();
    std::pair<int, int> getTaskStats();
    void saveChatId(const std::string& chat_id);
    std::vector<std::string> getAllChatIds() const;
    void unlinkAllAccounts();
    void deleteAllChatIds();
    void addColumnIfNotExists(const std::string& table, const std::string& column, const std::string& type);
    bool tableExists(const std::string& tableName);
    std::string getFirstChatId() const;
private:
    sqlite3* db_;  ///< SQLite database connection handle

    void executeQuery(const std::string& sql, const std::vector<std::string>& params = {});
    void throwOnError(int rc, const std::string& context) const;
    void executeTaskQuery(const std::string& sql, const Task& task);
    
    void bindTaskParameters(sqlite3_stmt* stmt, const Task& task);
    Task mapTaskFromRow(sqlite3_stmt* stmt);
};

#endif