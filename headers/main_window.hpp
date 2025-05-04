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
#include "telegram_bot.hpp"
#include "telegram_notifier.hpp"
#include <QSettings>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow();

private slots:
    void onAddButtonClicked();
    void onTaskDoubleClicked(QListWidgetItem* item);
    void onFilterChanged(int index);
    void onTelegramSettingsSaved(); 
    void onTestConnectionClicked();

private:
    DatabaseManager db_;
    std::vector<Task> tasks;
    std::vector<TaskTemplate> templates;

    QListWidget* taskList;
    QLineEdit* titleInput;
    QTextEdit* descInput;
    QComboBox* filterCombo;
    QPushButton* addButton;

    QDockWidget* telegramDock;
    QAction* toggleTelegramAction;
    QLineEdit* telegramTokenInput;
    QLineEdit* telegramChatIdInput;
    QPushButton* saveTelegramButton;
    QPushButton* testTelegramButton;
    std::unique_ptr<TelegramBot> telegramBot;
    std::unique_ptr<TelegramNotifier> notifier;
    
    QAction* tasksAction;
    QAction* templatesAction;
    QTabWidget* tasksTabs;
    QWidget* templatesTab;
    QTabWidget* mainTabs;

    void addTaskToList(const Task& task);
    void updateTaskInList(QListWidgetItem* item, const Task& task);
    void loadTelegramSettings();
    void closeEvent(QCloseEvent* event) override;
    void readSettings();
    void resizeEvent(QResizeEvent* event) override;
    void initTelegramUI();
    void initToolbar();
    void initTaskList();
    void initTemplateUI(QWidget* tab);
    void initActiveTasksUI(QWidget* tab);
    void loadTasksFromDB();
};

#endif