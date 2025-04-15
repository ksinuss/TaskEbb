#ifndef DATABASE_MANAGER_HPP
#define DATABASE_MANAGER_HPP

#include <sqlite3.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <functional>

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

    /**
     * @brief Saves a task to the specified table.
     * @tparam T Type of the task.
     * @param task Task object to be saved.
     * @param table_name Name of the target table.
     * @param bindParameters Callback function to bind task parameters to the SQL statement.
     */
    template <typename T>
    void saveTask(
        const T& task,
        const std::string& table_name,
        const std::function<void(sqlite3_stmt*, const T&)>& bindParameters
    );

    /**
     * @brief Retrieves all tasks from the specified table.
     * @tparam T Type of the task.
     * @param table_name Name of the source table.
     * @param rowMapper Callback function to map SQL result rows to task objects.
     * @return Vector of tasks.
     */
    template <typename T>
    std::vector<T> getAllTasks(
        const std::string& table_name,
        const std::function<T(sqlite3_stmt*)>& rowMapper
    );

    /**
     * @brief Updates a task in the specified table.
     * @tparam T Type of the task.
     * @param task Updated task object.
     * @param table_name Name of the target table.
     * @param bindParameters Callback function to bind task parameters to the SQL statement.
     */
    template <typename T>
    void updateTask(
        const T& task,
        const std::string& table_name,
        const std::function<void(sqlite3_stmt*, const T&)>& bindParameters
    );

    /**
     * @brief Deletes a task by its ID.
     * @param id Unique identifier of the task.
     * @param table_name Name of the target table.
     */
    void deleteTask(const std::string& id, const std::string& table_name);

    /**
     * @brief Creates a table using the provided SQL schema.
     * @param create_sql SQL CREATE TABLE statement.
     */
    void createTable(const std::string& create_sql);

private:
    sqlite3* db_;  ///< SQLite database connection handle

    void executeQuery(const std::string& sql);
    void throwOnError(int rc, const std::string& context);
};

#endif