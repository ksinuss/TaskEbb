#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidgetItem>
#include "task.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow();
    void updateTaskList();

private slots:
    void addTask();

private:
    QListWidget* taskList;
    QLineEdit* titleInput;
    QTextEdit* descInput;
    std::vector<Task> tasks;
};

#endif