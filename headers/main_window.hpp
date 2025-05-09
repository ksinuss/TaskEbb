#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include "task.hpp"
#include "database_manager.hpp"
#include "telegram_bot.hpp"
#include "telegram_notifier.hpp"
#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit> 
#include <QTextEdit>     
#include <QComboBox>    
#include <QPushButton> 
#include <vector>
#include <QSettings>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QStackedWidget>

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

    QListWidget* taskList = nullptr;
    QLineEdit* titleInput;
    QTextEdit* descInput;
    QComboBox* filterCombo;
    QPushButton* addButton;

    QAction* tasksAction;
    QAction* templatesAction;
    QTabWidget* tasksTabs;
    QTabWidget* templatesTabs;
    QStackedWidget* mainStack;

    QDockWidget* telegramDock;
    QAction* toggleTelegramAction;
    QLineEdit* telegramTokenInput;
    QLineEdit* telegramChatIdInput;
    QPushButton* saveTelegramButton;
    QPushButton* testTelegramButton;
    std::unique_ptr<TelegramBot> telegramBot;
    std::unique_ptr<TelegramNotifier> notifier;
    
    void addTaskToList(const Task& task);
    void loadTasksFromDB();
    void updateTaskInList(QListWidgetItem* item, const Task& task);
    void loadTelegramSettings();
    void closeEvent(QCloseEvent* event) override;
    void readSettings();
    void resizeEvent(QResizeEvent* event) override;
    void initTelegramUI();
    void initToolbar();
    void initTemplateUI(QWidget* tab);
    void initActiveTasksUI(QWidget* tab);
    void initStatsUI(QWidget* tab);
};

#endif