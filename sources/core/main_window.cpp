#include "main_window.hpp"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    auto* centralWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(centralWidget);

    taskList = new QListWidget(this);
    layout->addWidget(taskList);
    
    titleInput = new QLineEdit(this);
    titleInput->setPlaceholderText("Название задачи");
    layout->addWidget(titleInput);

    descInput = new QTextEdit(this);
    descInput->setPlaceholderText("Описание");
    layout->addWidget(descInput);

    auto* addButton = new QPushButton("➕ Добавить", this);
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addTask);
    layout->addWidget(addButton);

    setCentralWidget(centralWidget);
    setWindowTitle("TaskEbb");
    resize(400, 500);
}

MainWindow::~MainWindow() {}

void MainWindow::updateTaskList() {
    taskList->clear();
    for (const auto& task : tasks) {
        auto* item = new QListWidgetItem(QString::fromStdString(task.get_title()));
        item->setData(Qt::UserRole, QVariant(QString::fromStdString(task.get_id())));
        item->setCheckState(task.is_completed() ? Qt::Checked : Qt::Unchecked);
        taskList->addItem(item);
    }
}

void MainWindow::addTask() {
    std::string title = titleInput->text().toStdString();
    std::string description = descInput->toPlainText().toStdString();
    Task newTask(title, description);
    tasks.push_back(newTask);
    updateTaskList();
    titleInput->clear();
    descInput->clear();
}