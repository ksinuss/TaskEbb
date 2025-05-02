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
    void addTaskToList(const Task& task);
    void updateTaskInList(QListWidgetItem* item, const Task& task);

    DatabaseManager db_;
    std::vector<Task> tasks;

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
    
    void initTelegramUI();
    void loadTelegramSettings();
    void initToolbar();
    void closeEvent(QCloseEvent* event) override;
    void readSettings();
    void initTaskList();
    void resizeEvent(QResizeEvent* event) override;

    QTabWidget* mainTabs;
};

#endif