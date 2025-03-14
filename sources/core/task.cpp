#ifndef TASK_HPP
#define TASK_HPP

// #include <string>
// #include <chrono>

class Task {
public:
    Task(const std::string& title, const std::string& description = "");
    ~Task() = default;

    // getters
    std::string get_id() const;
    std::string get_title() const;
    std::string get_description() const;
    bool is_completed() const;

}