#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include "task.hpp"
#include "database_manager.hpp"
#include "telegram_bot.hpp"
#include "config_manager.hpp"
#include <vector>
#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit> 
#include <QTextEdit>     
#include <QComboBox>    
#include <QPushButton> 
#include <QSettings>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QStackedWidget>
#include <QLabel>
#include <QSpinBox>
#include <QDateEdit>
#include <QTimeEdit>
#include <QCheckBox>
#include <QHBoxLayout>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(ConfigManager& config, DatabaseManager& db, QWidget* parent = nullptr);
    virtual ~MainWindow();
    
    void handleChatIdRegistered();

signals:
    void chatIdUpdated();
    
private slots:
    void onAddButtonClicked();
    void onTaskDoubleClicked(QListWidgetItem* item);
    void onFilterChanged(int index);
    void onTelegramSettingsSaved();

private:
    ConfigManager& config_;
    DatabaseManager& db_;
    std::vector<Task> tasks;
    std::vector<TaskTemplate> templates;

    QListWidget* taskList = nullptr;
    QLineEdit* titleInput;
    QTextEdit* descInput;
    QSpinBox* intervalInput;
    QComboBox* filterCombo;
    QPushButton* addButton;

    QAction* tasksAction;
    QAction* templatesAction;
    QTabWidget* tasksTabs;
    QTabWidget* templatesTabs;
    QStackedWidget* mainStack;

    QDockWidget* telegramDock;
    QAction* toggleTelegramAction;
    QLabel* instructionLabel;
    QLineEdit* chatIdInput;
    QLabel* statusLabel;
    QPushButton* saveManualButton;
    QPushButton* unlinkButton;
    std::unique_ptr<TelegramBot> telegramBot;

    QComboBox* taskTypeCombo;
    QDateTimeEdit* deadlineEdit;
    
    QDateEdit* dateEdit;
    QTimeEdit* timeEdit;
    QCheckBox* timeCheckbox;
    QHBoxLayout* datetimeLayout;
    
    QDateEdit* endDateEdit;
    QCheckBox* endDateCheckbox;

    QWidget* deadlineContainer = nullptr;
    QWidget* recurringContainer = nullptr;
    
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
    void unlinkTelegramAccount();
    void updateUIForLinkedStatus(bool isLinked);
    void updateTelegramStatus();
    void formatTaskItem(QListWidgetItem* item, const Task& task);
    void initUI();
    void initTaskInputFields();
    void setupDeadlineFields();
    void updateStatsUI();
};

#endif