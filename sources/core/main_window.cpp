#include "main_window.hpp"
#include "telegram_bot.hpp"
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
#include <QTimer>
#include <QCoreApplication>
#include <QSpinBox>
#include <QStackedLayout>
#include <QtCharts>
#include <QPainter>
#include <QRegularExpressionValidator>

MainWindow::MainWindow(ConfigManager& config, DatabaseManager& db, QWidget* parent) : QMainWindow(parent), config_(config), db_(db) {
    QCoreApplication::setOrganizationName("ksinuss");
    QCoreApplication::setApplicationName("TaskEbb");
    
    mainStack = new QStackedWidget(this);
    setCentralWidget(mainStack);

    tasksTabs = new QTabWidget();
    QWidget* activeTasksTab = new QWidget();
    initActiveTasksUI(activeTasksTab);
    tasksTabs->addTab(activeTasksTab, "Активные задачи");
    
    QWidget* statsTab = new QWidget();
    initStatsUI(statsTab);
    tasksTabs->addTab(statsTab, "Статистика");
    mainStack->addWidget(tasksTabs);

    templatesTabs = new QTabWidget();
    QWidget* templatesTab = new QWidget();
    initTemplateUI(templatesTab);
    templatesTabs->addTab(templatesTab, "Управление шаблонами");
    mainStack->addWidget(templatesTabs);

    mainStack->setCurrentIndex(0);

    initToolbar();
    initTelegramUI();
    loadTelegramSettings();
    updateTelegramStatus();

    resize(800, 600);

    telegramBot = std::make_unique<TelegramBot>(config_, db_);
    connect(telegramBot.get(), &TelegramBot::chatIdRegistered, this, &MainWindow::handleChatIdRegistered);

    try {
        telegramBot->start();
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Ошибка", QString::fromStdString(e.what()));
        exit(1);
    }
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

void MainWindow::updateTaskInList(QListWidgetItem* item, const Task& task) {
    QString status = task.is_completed() ? "[✓]" : "[ ]";
    item->setText(QString("%1 %2").arg(status, QString::fromStdString(task.get_title())));
}

void MainWindow::onTelegramSettingsSaved() {
    QString chatId = chatIdInput->text().trimmed();
    if(!chatId.isEmpty()) {
        try {
            db_.saveChatId(chatId.toStdString());
            updateTelegramStatus();
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить Chat ID: " + QString(e.what()));
        }
    }
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

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (telegramDock && telegramDock->isVisible()) {
        int minWidth = qMin(width() * 0.3, 300.0);
        telegramDock->setMinimumWidth(minWidth);
    }
}

void MainWindow::initTelegramUI() {
    QScrollArea* scrollArea = new QScrollArea(this);
    QWidget* container = new QWidget(scrollArea);
    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    instructionLabel = new QLabel(container);
    instructionLabel->setWordWrap(true);
    
    chatIdInput = new QLineEdit(container);
    
    saveManualButton = new QPushButton("Привязать вручную", container);
    statusLabel = new QLabel("Статус: Проверка...", container);
    QPushButton* refreshButton = new QPushButton("Обновить статус", container);
    QPushButton* unlinkButton = new QPushButton("Отвязать аккаунт", container);
    
    statusLabel->setStyleSheet("QLabel { color: #2E7D32; font-weight: 600; }");
    unlinkButton->setStyleSheet("QPushButton { color: #C62828; }");

    QFormLayout* formLayout = new QFormLayout();
    formLayout->addRow(instructionLabel);
    formLayout->addRow("Chat ID:", chatIdInput);
    formLayout->addRow(saveManualButton);
    formLayout->addRow(statusLabel);
    formLayout->addRow(refreshButton);
    formLayout->addRow(unlinkButton);
    
    mainLayout->addLayout(formLayout);
    scrollArea->setWidget(container);
    scrollArea->setWidgetResizable(true);
    
    telegramDock = new QDockWidget("Настройки Telegram", this);
    telegramDock->setWidget(scrollArea);
    telegramDock->setFeatures(
        QDockWidget::DockWidgetClosable | 
        QDockWidget::DockWidgetMovable | 
        QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::RightDockWidgetArea, telegramDock);
    telegramDock->setVisible(false);
    
    connect(saveManualButton, &QPushButton::clicked, this, &MainWindow::onTelegramSettingsSaved);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::updateTelegramStatus);
    connect(unlinkButton, &QPushButton::clicked, this, &MainWindow::unlinkTelegramAccount);
    
    updateUIForLinkedStatus(false);
    updateTelegramStatus();
}

void MainWindow::loadTelegramSettings() {
    QSettings settings;
    QString savedChatId = settings.value("telegram/chat_id", "").toString();
    if (chatIdInput) {
        chatIdInput->setText(savedChatId);
    }
}

void MainWindow::initToolbar() {
    QToolBar* toolbar = new QToolBar("Панель управления", this);
    toolbar->setMovable(false);
    addToolBar(toolbar);

    QAction* tasksAction = new QAction("Задачи", this);
    QAction* templatesAction = new QAction("Шаблоны", this);
    
    connect(tasksAction, &QAction::triggered, this, [this]() {
        mainStack->setCurrentIndex(0);
    });
    connect(templatesAction, &QAction::triggered, this, [this]() {
        mainStack->setCurrentIndex(1);
    });

    toolbar->addAction(tasksAction);
    toolbar->addAction(templatesAction);

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

    addToolBar(toolbar);
}

void MainWindow::loadTasksFromDB() {
    try {
        tasks = db_.getAllTasks("tasks", [](sqlite3_stmt* stmt) {
            Task task(
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)), 
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))
            );
            task.mark_completed(sqlite3_column_int(stmt, 3) == 1);
            task.set_interval(std::chrono::hours(sqlite3_column_int(stmt, 4)));
            return task;
        });
        for (const auto& task : tasks) {
            addTaskToList(task);
        }
    } catch (...) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить задачи из БД");
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

void MainWindow::initTemplateUI(QWidget* tab) {
    QVBoxLayout* mainLayout = new QVBoxLayout(tab);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    QComboBox* templateCombo = new QComboBox(tab);
    templateCombo->addItem("Выберите шаблон");
    mainLayout->addWidget(templateCombo);

    QFormLayout* form = new QFormLayout();
    QLineEdit* tmplTitleInput = new QLineEdit(tab);
    QTextEdit* tmplDescInput = new QTextEdit(tab);
    QSpinBox* intervalInput = new QSpinBox(tab);
    intervalInput->setMinimum(1);
    intervalInput->setMaximum(24 * 365);
    intervalInput->setValue(24);

    form->addRow("Название шаблона:", tmplTitleInput);
    form->addRow("Описание:", tmplDescInput);
    form->addRow("Интервал (часы):", intervalInput);
    mainLayout->addLayout(form);

    QPushButton* saveTmplButton = new QPushButton("Сохранить шаблон", tab);
    mainLayout->addWidget(saveTmplButton);

    QListWidget* tmplList = new QListWidget(tab);
    mainLayout->addWidget(tmplList);

    try {
        templates = db_.getAllTemplates();
        for (const auto& tmpl : templates) {
            templateCombo->addItem(QString::fromStdString(tmpl.get_title()));
            tmplList->addItem(QString::fromStdString(tmpl.get_title()));
        }
    } catch (...) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить шаблоны");
    }

    connect(templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=, this](int index) {
        if (index <= 0) {
            tmplTitleInput->clear();
            tmplDescInput->clear();
            intervalInput->setValue(24);
            return;
        }
        const TaskTemplate& selected = templates[index - 1];
        tmplTitleInput->setText(QString::fromStdString(selected.get_title()));
        tmplDescInput->setText(QString::fromStdString(selected.get_description()));
        intervalInput->setValue(selected.get_interval_hours());
    });
    connect(saveTmplButton, &QPushButton::clicked, this, [=, this]() {
        if (tmplTitleInput->text().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Название шаблона обязательно!");
            return;
        }
        try {
            TaskTemplate newTmpl(
                tmplTitleInput->text().toStdString(),
                tmplDescInput->toPlainText().toStdString(),
                intervalInput->value()
            );
            db_.saveTemplate(newTmpl);
            templates.push_back(newTmpl);
            templateCombo->addItem(tmplTitleInput->text());
            tmplList->addItem(tmplTitleInput->text());

            tmplTitleInput->clear();
            tmplDescInput->clear();
            intervalInput->setValue(24);
        } catch (...) {
            QMessageBox::critical(this, "Ошибка", "Ошибка сохранения шаблона");
        }
    });
}

void MainWindow::initActiveTasksUI(QWidget* tab) {
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QFormLayout* form = new QFormLayout();
    titleInput = new QLineEdit(this);
    descInput = new QTextEdit(this);
    addButton = new QPushButton("Добавить задачу", this);
    filterCombo = new QComboBox(this);
    filterCombo->addItems({"Все задачи", "Выполненные", "Невыполненные"});

    form->addRow("Заголовок:", titleInput);
    form->addRow("Описание:", descInput);

    taskList = new QListWidget(tab);
    taskList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(taskList, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QListWidgetItem* item = taskList->itemAt(pos);
        if (!item) {
            return;
        }
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

    connect(addButton, &QPushButton::clicked, this, &MainWindow::onAddButtonClicked);
    connect(taskList, &QListWidget::itemDoubleClicked, this, &MainWindow::onTaskDoubleClicked);
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterChanged);
}

void MainWindow::initStatsUI(QWidget* tab) {
    QVBoxLayout* layout = new QVBoxLayout(tab);
    
    auto [completed, pending] = db_.getTaskStats();
    
    auto* series = new QPieSeries();
    series->append("Выполнено (" + QString::number(completed) + ")", completed);
    series->append("Не выполнено (" + QString::number(pending) + ")", pending);
    
    series->slices().at(0)->setColor(QColor("#4CAF50"));
    series->slices().at(1)->setColor(QColor("#F44336"));
    
    auto* chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Статистика выполнения задач");
    chart->legend()->setAlignment(Qt::AlignBottom);
    
    auto* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    
    layout->addWidget(chartView);
    
    QPushButton* refreshButton = new QPushButton("Обновить", this);
    connect(refreshButton, &QPushButton::clicked, this, [this, chart]() {
        auto [completed, pending] = db_.getTaskStats();
        auto* series = static_cast<QPieSeries*>(chart->series().at(0));
        series->clear();
        series->append("Выполнено", completed)->setColor(QColor("#4CAF50"));
        series->append("Не выполнено", pending)->setColor(QColor("#F44336"));
    });
    
    layout->addWidget(refreshButton);
}

void MainWindow::handleChatIdRegistered() {
    updateTelegramStatus();
    QMessageBox::information(this, "Успех", "Telegram аккаунт привязан!");
}

void MainWindow::updateTelegramStatus() {
    try {
        auto chatIds = db_.getAllChatIds();
        bool isLinked = !chatIds.empty();
        
        if(isLinked) {
            statusLabel->setText("✔️ Привязан: " + QString::fromStdString(chatIds[0]));
            statusLabel->setStyleSheet("color: #2E7D32; font-weight: 600;");
            updateUIForLinkedStatus(true);
        } else {
            statusLabel->setText("❌ Не привязан");
            statusLabel->setStyleSheet("color: #C62828; font-weight: 600;");
            updateUIForLinkedStatus(false);
        }
        
    } catch(const std::exception& e) {
        statusLabel->setText("⚠️ Ошибка: " + QString(e.what()));
        statusLabel->setStyleSheet("color: #EF6C00; font-weight: 600;");
    }
}

void MainWindow::updateUIForLinkedStatus(bool isLinked) {
    Q_ASSERT_X(instructionLabel, "updateUIForLinkedStatus", "instructionLabel is nullptr!");
    Q_ASSERT_X(chatIdInput, "updateUIForLinkedStatus", "chatIdInput is nullptr!");
    Q_ASSERT_X(saveManualButton, "updateUIForLinkedStatus", "saveManualButton is nullptr!");
    
    instructionLabel->setVisible(!isLinked);
    chatIdInput->setVisible(!isLinked);
    saveManualButton->setVisible(!isLinked);
    
    QString instructionText = 
        "<b>Способы привязки:</b><br>"
        "1. Перейдите к <a href='https://t.me/task_ebb_bot'>боту</a> и отправьте /start<br>"
        "2. Или введите Chat ID вручную";
    instructionLabel->setText(instructionText);
    instructionLabel->setOpenExternalLinks(true);
}

void MainWindow::unlinkTelegramAccount() {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Подтверждение", 
        "Вы уверены, что хотите отвязать аккаунт?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        try {
            db_.deleteAllChatIds();
            updateTelegramStatus();
            updateUIForLinkedStatus(false);
            QMessageBox::information(this, "Успех", "Аккаунт отвязан!");
        } catch (...) {
            QMessageBox::critical(this, "Ошибка", "Не удалось отвязать аккаунт");
        }
    }
}