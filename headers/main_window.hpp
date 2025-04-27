#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit> 
#include <QTextEdit>     
#include <QComboBox>    
#include <QPushButton> 
#include <vector>
#include "task.hpp"
#include "database_manager.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow();

private slots:
    void onAddButtonClicked();
    void onTaskDoubleClicked(QListWidgetItem* item);
    void onFilterChanged(int index);

private:
    void addTaskToList(const Task& task);
    void updateTaskInList(QListWidgetItem* item, const Task& task);

    DatabaseManager db_;
    std::vector<Task> tasks;

    QListWidget* taskList;
    QLineEdit* titleInput;
    QTextEdit* descInput;
    QComboBox* filterCombo;
    QPushButton* addButton;
};

#endif