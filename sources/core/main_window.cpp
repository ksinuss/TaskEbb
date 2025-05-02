#include "main_window.hpp"
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QSettings>
#include <QTimer>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), db_("tasks.db") {
    QCoreApplication::setOrganizationName("ksinuss");
    QCoreApplication::setApplicationName("TaskEbb");
    
    QWidget* centralTabWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralTabWidget);

    titleInput = new QLineEdit(this);
    descInput = new QTextEdit(this);
    addButton = new QPushButton("Добавить задачу", this);
    taskList = new QListWidget(this);
    filterCombo = new QComboBox(this);
    filterCombo->addItems({"Все задачи", "Выполненные", "Невыполненные"});

    QFormLayout* form = new QFormLayout();
    form->addRow("Заголовок:", titleInput);
    form->addRow("Описание:", descInput);

    taskList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(taskList, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QListWidgetItem* item = taskList->itemAt(pos);
        if (!item) return;

        QMenu menu;
        QAction* showDetails = menu.addAction("Показать описание");
        connect(showDetails, &QAction::triggered, this, [this, item]() {
            QMessageBox::information(this, "Описание", item->toolTip());
        });
        menu.exec(taskList->viewport()->mapToGlobal(pos));
    });

    layout->addLayout(form);
    layout->addWidget(addButton);
    layout->addWidget(filterCombo);
    layout->addWidget(taskList);

    mainTabs = new QTabWidget(this);
    mainTabs->addTab(centralTabWidget, "Активные задачи");
    mainTabs->addTab(new QWidget(), "Статистика");
    setCentralWidget(mainTabs);

    initToolbar();
    initTelegramUI();
    loadTelegramSettings();

    resize(800, 600);

    try {
        db_.initialize();
        tasks = db_.getAllTasks("tasks", [](sqlite3_stmt* stmt) {
            Task task(
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)), // title
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))  // description
            );

            task.mark_completed(sqlite3_column_int(stmt, 3) == 1); // completed
            task.set_interval(std::chrono::hours(sqlite3_column_int(stmt, 4))); // interval

            return task;
        });
        for (const auto& task : tasks) {
            addTaskToList(task);
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Ошибка", e.what());
    } catch (...) {
        QMessageBox::critical(this, "Ошибка", "Неизвестная ошибка");
    }

    connect(addButton, &QPushButton::clicked, this, &MainWindow::onAddButtonClicked);
    connect(taskList, &QListWidget::itemDoubleClicked, this, &MainWindow::onTaskDoubleClicked);
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterChanged);
}

MainWindow::~MainWindow() {
    if (telegramBot) {
        telegramBot->stop();
    }
}

void MainWindow::onAddButtonClicked() {
    QString title = titleInput->text();
    QString description = descInput->toPlainText();

    if (title.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Заголовок задачи не может быть пустым.");
        return;
    }

    try {
        Task task(title.toStdString(), description.toStdString());
        db_.saveTask(task, "tasks", [](sqlite3_stmt* stmt, const Task& task) {
            sqlite3_bind_text(stmt, 1, task.get_id().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, task.get_title().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, task.get_description().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 4, task.is_completed() ? 1 : 0);
            sqlite3_bind_int(stmt, 5, task.get_interval().count());
        });
        tasks.push_back(task);
        addTaskToList(task);
        db_.logAction("ADD", task.get_id(), "Создана задача: " + task.get_title());
        titleInput->clear();
        descInput->clear();
    } catch (...) {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить задачу.");
    }
}

void MainWindow::onTaskDoubleClicked(QListWidgetItem* item) {
    int row = taskList->row(item);
    Task& task = tasks[row];

    QDialog dialog(this);
    QFormLayout layout(&dialog);
    
    QLineEdit titleEdit(QString::fromStdString(task.get_title()));
    QTextEdit descEdit(QString::fromStdString(task.get_description()));
    layout.addRow("Заголовок:", &titleEdit);
    layout.addRow("Описание:", &descEdit);

    QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    layout.addRow(&buttons);

    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        task.set_title(titleEdit.text().toStdString());
        task.set_description(descEdit.toPlainText().toStdString());
        
        try {
            db_.updateTask(task, "tasks", [](sqlite3_stmt* stmt, const Task& task) {
                sqlite3_bind_text(stmt, 1, task.get_title().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, task.get_description().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt, 3, task.is_completed() ? 1 : 0);
                sqlite3_bind_int(stmt, 4, task.get_interval().count());
                sqlite3_bind_text(stmt, 5, task.get_id().c_str(), -1, SQLITE_TRANSIENT);
            });
            updateTaskInList(item, task);
            db_.logAction("EDIT", task.get_id(), "Изменена задача: " + task.get_title());
        } catch (...) {
            QMessageBox::critical(this, "Ошибка", "Не удалось обновить задачу.");
        }
    }
}

void MainWindow::onFilterChanged(int index) {
    taskList->clear();
    for (const auto& task : tasks) {
        bool show = false;
        switch (index) {
            case 0: 
                show = true; 
                break; 
            case 1: 
                show = task.is_completed(); 
                break; 
            case 2: 
                show = !task.is_completed(); 
                break; 
        }
        if (show) addTaskToList(task);
    }
}

void MainWindow::addTaskToList(const Task& task) {
    QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(task.get_title()), taskList);
    
    item->setCheckState(task.is_completed() ? Qt::Checked : Qt::Unchecked);
    
    QString description = QString::fromStdString(task.get_description());
    item->setToolTip(description.isEmpty() ? "Описание отсутствует" : "Описание: " + description);

    item->setData(Qt::UserRole, QString::fromStdString(task.get_id()));

    if (task.is_completed()) {
        item->setForeground(Qt::gray);
        item->setFont(QFont("Arial", 10, QFont::StyleItalic));
    }

    connect(taskList, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
        QString taskId = item->data(Qt::UserRole).toString();
        bool completed = (item->checkState() == Qt::Checked);
        
        for (auto& task : tasks) {
            if (task.get_id() == taskId.toStdString()) {
                task.mark_completed(completed);
                db_.updateTask(task, "tasks", [](sqlite3_stmt* stmt, const Task& t) {
                    sqlite3_bind_text(stmt, 1, t.get_title().c_str(), -1, SQLITE_TRANSIENT);  // title
                    sqlite3_bind_text(stmt, 2, t.get_description().c_str(), -1, SQLITE_TRANSIENT); // description
                    sqlite3_bind_int(stmt, 3, t.is_completed() ? 1 : 0); // is_completed
                    sqlite3_bind_int(stmt, 4, t.get_interval().count()); // interval_hours
                    sqlite3_bind_text(stmt, 5, t.get_id().c_str(), -1, SQLITE_TRANSIENT); // id (WHERE)
                });
                break;
            }
        }
    });
}

void MainWindow::updateTaskInList(QListWidgetItem* item, const Task& task) {
    QString status = task.is_completed() ? "[✓]" : "[ ]";
    item->setText(QString("%1 %2").arg(status, QString::fromStdString(task.get_title())));
}

void MainWindow::initTelegramUI() {
    QWidget* telegramSettings = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(telegramSettings);
    QFormLayout* telegramLayout = new QFormLayout();
    
    telegramTokenInput = new QLineEdit(this);
    telegramChatIdInput = new QLineEdit(this);
    telegramTokenInput->setMaximumWidth(250);
    telegramChatIdInput->setMaximumWidth(250);
    saveTelegramButton = new QPushButton("Сохранить", this);
    testTelegramButton = new QPushButton("Проверить подключение", this);
    
    telegramLayout->addRow("Токен бота:", telegramTokenInput);
    telegramLayout->addRow("Chat ID:", telegramChatIdInput);
    telegramLayout->addRow(saveTelegramButton);
    telegramLayout->addRow(testTelegramButton);
    telegramLayout->setContentsMargins(15, 15, 15, 15);
    telegramLayout->setSpacing(10);
    
    mainLayout->addLayout(telegramLayout);
    
    telegramDock = new QDockWidget("Настройки Telegram", this);
    telegramDock->setWidget(telegramSettings);
    telegramDock->setFeatures(
        QDockWidget::DockWidgetClosable | 
        QDockWidget::DockWidgetMovable | 
        QDockWidget::DockWidgetFloatable);
    telegramDock->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    telegramDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, telegramDock);

    connect(telegramDock, &QDockWidget::visibilityChanged, toggleTelegramAction, &QAction::setChecked);
    
    connect(saveTelegramButton, &QPushButton::clicked, this, &MainWindow::onTelegramSettingsSaved);
    connect(testTelegramButton, &QPushButton::clicked, this, &MainWindow::onTestConnectionClicked);
}

void MainWindow::loadTelegramSettings() {
    QSettings settings;
    QString token = settings.value("telegram/token", "").toString();
    QString chatId = settings.value("telegram/chat_id", "").toString();
    
    if (telegramTokenInput && telegramChatIdInput) {
        telegramTokenInput->setText(token);
        telegramChatIdInput->setText(chatId);
    }
}

void MainWindow::onTelegramSettingsSaved() {
    if (telegramTokenInput->text().isEmpty() || telegramChatIdInput->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Заполните все поля!");
        return;
    }

    QSettings settings;
    settings.setValue("telegram/token", telegramTokenInput->text());
    settings.setValue("telegram/chat_id", telegramChatIdInput->text());
    
    settings.sync();
    if (settings.status() != QSettings::NoError) {
        qDebug() << "Ошибка сохранения настроек!";
    }
    qDebug() << "Токен сохранен:" << settings.value("telegram/token");
    qDebug() << "Chat ID сохранен:" << settings.value("telegram/chat_id");

    try {
        if (telegramBot) {
            telegramBot->stop();
            telegramBot.reset();
        }
        
        telegramBot = std::make_unique<TelegramBot>(
            telegramTokenInput->text().toStdString(), 
            db_
        );
        telegramBot->start();
    } catch (const std::bad_alloc& e) {
        QMessageBox::critical(this, "Ошибка", "Недостаточно памяти для создания бота.");
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Ошибка", e.what());
    } catch (...) {
        QMessageBox::critical(this, "Ошибка", "Неизвестная ошибка при создании бота.");
    }
}

void MainWindow::onTestConnectionClicked() {
    if (telegramTokenInput->text().isEmpty() || telegramChatIdInput->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Заполните токен и chat_id!");
        return;
    }

    notifier = std::make_unique<TelegramNotifier>(
        telegramTokenInput->text().toStdString(),
        telegramChatIdInput->text().toStdString()
    );
    
    if (notifier->send_message("Тест подключения: успешно!")) {
        QMessageBox::information(this, "Успех", "Сообщение отправлено в Telegram!");
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось отправить сообщение.");
    }
}

void MainWindow::initToolbar() {
    QToolBar* toolbar = new QToolBar("Панель управления", this);
    toolbar->setMovable(false);
    addToolBar(Qt::TopToolBarArea, toolbar);

    QAction* tasksAction = new QAction("Задачи", this);
    toolbar->addAction(tasksAction);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolbar->addWidget(spacer);

    toggleTelegramAction = new QAction("Привязать Telegram", this);
    toggleTelegramAction->setCheckable(true);
    toolbar->addAction(toggleTelegramAction);

    connect(toggleTelegramAction, &QAction::toggled, this, [this](bool checked) {
        telegramDock->setVisible(checked);
        if(checked && telegramDock->isHidden()) {
            telegramDock->show();
        }
    });
}

void MainWindow::closeEvent(QCloseEvent* event) {
    QSettings settings;
    settings.setValue("telegramPanelVisible", telegramDock->isVisible());
    QTimer::singleShot(0, [this]() {
        db_.logAction("EXIT", "", "Приложение закрыто");
    });
    
    QMainWindow::closeEvent(event);
}

void MainWindow::readSettings() {
    QSettings settings;
    bool telegramVisible = settings.value("telegramPanelVisible", true).toBool();
    findChild<QDockWidget*>("Настройки Telegram")->setVisible(telegramVisible);
}

void MainWindow::initTaskList() {
    QWidget* taskTab = mainTabs->widget(0);
    QVBoxLayout* layout = new QVBoxLayout(taskTab);

    QListWidget* taskList = new QListWidget(taskTab);
    layout->addWidget(taskList);

    for (const auto& task : tasks) {
        QListWidgetItem* item = new QListWidgetItem(taskList);
        QWidget* taskWidget = new QWidget();
        
        QHBoxLayout* taskLayout = new QHBoxLayout(taskWidget);
        
        QLabel* title = new QLabel(QString::fromStdString(task.get_title()));
        
        QString description = QString::fromStdString(task.get_description());
        if (description.length() > 50) {
            description = description.left(47) + "...";
        }
        QLabel* desc = new QLabel(description);
        desc->setStyleSheet("color: gray; font-size: 10pt;");
        
        QLabel* deadline = new QLabel("До: 2024-12-31");
        deadline->setAlignment(Qt::AlignRight);
        
        taskLayout->addWidget(title);
        taskLayout->addWidget(desc);
        taskLayout->addWidget(deadline);
        
        item->setSizeHint(taskWidget->sizeHint());
        taskList->setItemWidget(item, taskWidget);
    }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (telegramDock && telegramDock->isVisible()) {
        telegramDock->setMaximumWidth(width() * 0.3);
    }
}