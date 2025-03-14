#ifndef TASK_HPP
#define TASK_HPP

#include <string>
#include <chrono> // точное время, генерация id

/**
 * @class Task
 * @brief Хранит информацию о задаче: название, описание, статус выполнения, интервал повторения
 */
class Task {
public:
    /**
     * @brief Construct a new Task object
     * @param title Название задачи (должно быть)
     * @param description Описание задачи (пустое по умолчанию)
     */
    Task(const std::string& title, const std::string& description = "");
    ~Task() = default;

    /**
     * @brief Get the id object
     * @return Уникальный идентификатор задачи
     */
    std::string get_id() const;

    /**
     * @brief Get the title object
     * @return Название задачи
     */
    std::string get_title() const;

    /**
     * @brief Get the description object
     * @return Описание задачи
     */
    std::string get_description() const;

    /**
     * @brief Возвращает статус выполнения задачи
     * @return true, если задача выполнена, иначе false 
     */
    bool is_completed() const;

    /**
     * @brief Set the interval object (повторения задачи)
     * @param interval Интервал (в часах)
     */
    void set_interval(std::chrono::hours interval);

    /**
     * @brief Отмечает статус выполнения задачи
     * @param status true, если выполнена, иначе false
     */
    void mark_completed(bool status);

private:
    std::string id_;                ///< Уникальный идентификатор задачи
    std::string title_;             ///< Название задачи
    std::string description_;       ///< Описание задачи
    bool is_completed_;             ///< Статус выполнения задачи
    std::chrono::hours interval_;   ///< Интервал повторения задачи
};

#endif