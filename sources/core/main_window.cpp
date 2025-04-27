#include "main_window.hpp"
#include <QMessageBox> 
#include <QDialog> 
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMenu>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), db_("tasks.db") {
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    titleInput = new QLineEdit(this);
    descInput = new QTextEdit(this);
    addButton = new QPushButton("Добавить задачу", this);
    taskList = new QListWidget(this);
    filterCombo = new QComboBox(this);

    filterCombo->addItem("Все задачи");
    filterCombo->addItem("Выполненные");
    filterCombo->addItem("Невыполненные");

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

    setCentralWidget(centralWidget);

    try {
        tasks = db_.getAllTasks("tasks", [](sqlite3_stmt* stmt) {
            std::string id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            std::string description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            bool completed = sqlite3_column_int(stmt, 3);
            int interval = sqlite3_column_int(stmt, 4);

            Task task(title, description);
            task.set_id(id);
            task.mark_completed(completed);
            task.set_interval(std::chrono::hours(interval));
            return task;
        });
        for (const auto& task : tasks) {
            addTaskToList(task);
        }
    } catch (...) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить задачи.");
    }

    connect(addButton, &QPushButton::clicked, this, &MainWindow::onAddButtonClicked);
    connect(taskList, &QListWidget::itemDoubleClicked, this, &MainWindow::onTaskDoubleClicked);
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterChanged);
}

MainWindow::~MainWindow() {}

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