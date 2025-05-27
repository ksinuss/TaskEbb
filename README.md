---

### **Техническая документация проекта TaskEbb**

---

## **Оглавление**
1. [Архитектура проекта](#архитектура-проекта)  
2. [Структура кода](#структура-кода)  
3. [Описание классов](#описание-классов)  
4. [Логика работы](#логика-работы)  
5. [Зависимости и настройки](#зависимости-и-настройки)  
6. [Примеры работы с API](#примеры-работы-с-api)  
7. [Обработка ошибок](#обработка-ошибок)  
8. [Советы по разработке](#советы-по-разработке)  

---

## **Архитектура проекта**
Проект построен по **модульному принципу**:
- **GUI**: Реализован на Qt 6 (Widgets, Charts). Отвечает за взаимодействие с пользователем.  
- **Логика**: Ядро приложения (классы `Task`, `TaskTemplate`, `DatabaseManager`).  
- **Хранение данных**: SQLite3 для локального сохранения задач и настроек.  
- **Взаимодействие**: Сигналы и слоты Qt для связи между GUI и логикой.

**Схема потоков данных**:
```plaintext
Пользователь -> GUI -> DatabaseManager -> SQLite  
GUI <- Данные задач/статистики <- DatabaseManager  
```

---

## **Структура кода**
```plaintext
final_project/  
├── CMakeLists.txt          # Конфигурация сборки   
├── headers/                # Заголовочные файлы 
│   ├── task.hpp      
│   ├── main_window.hpp    
│   └── ...  
├── sources/  
│   ├── main.cpp 
│   ├── core/               # Основная логика  
│   │   ├── task.cpp        
│   │   ├── main_window.cpp  
│   │   ├── database_manager.cpp  
│   │   └── ...  
│   └── icons/               # Изображения 
│       └── ...  
├── tests/                 # Модульные тесты  
├── config/                # Конфигурационные файлы  
└── doctest/               # Библиотека для доктестов 
```

---

## **Описание классов**
### **Класс `Task`**
**Назначение**: Хранение данных о задаче.  
**Поля**:
- `title` (string): Название задачи.  
- `description` (string): Подробное описание.  
- `type` (enum: OneTime, Deadline, Recurring): Тип задачи.  
- `deadline` (QDateTime): Дата выполнения (для Deadline).  

**Методы**:
| Метод                     | Описание                          | Пример использования               |
|---------------------------|-----------------------------------|------------------------------------|
| `mark_completed(bool)`    | Отметить выполнение задачи        | `task.mark_completed(true);`       |
| `set_deadline(QDateTime)` | Установить дедлайн                | `task.set_deadline(QDateTime(...))`|
| `is_recurring() -> bool`  | Проверить, периодическая ли задача | `if (task.is_recurring()) { ... }` |

---

### **Класс `DatabaseManager`**
**Назначение**: Управление взаимодействием с SQLite.  
**Методы**:
- `saveTask(const Task&)`: Сохраняет задачу в БД.  
- `getAllTasks() -> vector<Task>`: Возвращает список всех задач.  
- `getTaskStats() -> pair<int, int>`: Статистика (выполнено/невыполнено).  

**Пример SQL-запроса**:
```sql
INSERT INTO tasks (id, title, is_completed) 
VALUES ('123', 'Купить молоко', 0);
```

---

### **Техническая документация: Другие классы**

---

## **Класс `ConfigManager`**
**Назначение**: Управление конфигурацией приложения (чтение/запись настроек из файла `config.ini`).  
**Файлы**: `config_manager.cpp`, `config_manager.hpp`.

### **Ключевые поля**
- `config_path` (string): Путь к файлу конфигурации.  
- `default_path` (string): Значение по умолчанию (`config/config.ini`).

### **Методы**
| Метод                          | Описание                                  | Пример использования               |
|--------------------------------|-------------------------------------------|------------------------------------|
| `get_bot_token() -> string`    | Возвращает токен Telegram-бота           | `string token = cfg.get_bot_token()` |
| `get_db_path() -> string`      | Возвращает путь к БД                     | `string db_path = cfg.get_db_path()` |
| `read_key(section, key) -> string` | Чтение значения ключа из секции          | `read_key("Telegram", "BotToken")` |

**Пример файла `config.ini`**:
```ini
[Telegram]
BotToken = YOUR_TELEGRAM_BOT_TOKEN

[Database]
Path = tasks.db
```

---

## **Класс `TelegramBot`**
**Назначение**: Взаимодействие с Telegram API для отправки уведомлений.  
**Файлы**: `telegram_bot.cpp`, `telegram_bot.hpp`.

### **Ключевые поля**
- `bot_token` (string): Токен бота, полученный от `ConfigManager`.  
- `running` (bool): Флаг активности потока опроса сервера Telegram.  
- `polling_thread` (std::thread): Поток для асинхронного получения сообщений.

### **Методы**
| Метод                          | Описание                                  |
|--------------------------------|-------------------------------------------|
| `start()`                      | Запускает поток `pollingLoop`.           |
| `stop()`                       | Останавливает поток.                     |
| `send_message(text, chat_id)`  | Отправляет сообщение в Telegram.         |
| `pollingLoop()`                | Цикл опроса сервера Telegram на новые сообщения. |

**Пример использования**:
```cpp
ConfigManager cfg;
TelegramBot bot(cfg, db);
bot.start(); // Запуск бота
```

**Зависимости**:  
- `libcurl` для HTTP-запросов.  
- `nlohmann/json` для парсинга ответов API.

---

## **Класс `TaskTemplate`**
**Назначение**: Создание шаблонов для генерации периодических задач.  
**Файлы**: `task_template.cpp`, `task_template.hpp`.

### **Ключевые поля**
- `base_task` (Task): Базовая задача, на основе которой создаются экземпляры.  
- `custom_interval_hours` (int): Интервал повторения в часах.  
- `last_generated` (time_t): Время последней генерации задачи.

### **Методы**
| Метод                          | Описание                                  |
|--------------------------------|-------------------------------------------|
| `generate_tasks(up_to_timestamp)` | Генерирует задачи до указанного времени. |
| `get_step() -> time_t`         | Возвращает интервал повторения в секундах. |

**Пример**:
```cpp
TaskTemplate tmpl("Еженедельный отчет", "Сбор данных", 168); // 168 часов = 7 дней
auto tasks = tmpl.generate_tasks(time(nullptr) + 604800); // Генерация на следующую неделю
```

---

## **Класс `PeriodicTracker`**
**Назначение**: Отслеживание времени выполнения периодических задач.  
**Файлы**: `periodic_tracker.cpp`, `periodic_tracker.hpp`.

### **Ключевые поля**
- `first_execution` (std::optional<TimePoint>): Время первого выполнения.  
- `second_execution` (std::optional<TimePoint>): Время второго выполнения.  
- `interval` (std::optional<seconds>): Рассчитанный интервал между выполнениями.

### **Методы**
| Метод                          | Описание                                  |
|--------------------------------|-------------------------------------------|
| `mark_execution(timestamp)`    | Фиксирует время выполнения.              |
| `get_next_execution_time()`    | Возвращает время следующего выполнения.  |
| `is_interval_set() -> bool`    | Проверяет, установлен ли интервал.       |

**Пример**:
```cpp
PeriodicTracker tracker;
tracker.mark_execution(std::chrono::system_clock::now()); // Первое выполнение
// ... через 24 часа ...
tracker.mark_execution(std::chrono::system_clock::now()); // Второе выполнение
auto next_time = tracker.get_next_execution_time(); // Через 24 часа
```

---

## **Класс `MainWindow`**
**Назначение**: Графический интерфейс пользователя (Qt Widgets).  
**Файлы**: `main_window.cpp`, `main_window.hpp`.

### **Ключевые поля**
- `tasks` (vector<Task>): Список задач.  
- `db` (DatabaseManager): Объект для работы с БД.  
- `telegramBot` (unique_ptr<TelegramBot>): Указатель на Telegram-бота.

### **Методы**
| Метод                          | Описание                                  |
|--------------------------------|-------------------------------------------|
| `initUI()`                     | Инициализация интерфейса.                |
| `onAddButtonClicked()`         | Обработчик добавления задачи.            |
| `updateStatsUI()`              | Обновление диаграммы статистики.         |
| `loadTasksFromDB()`            | Загрузка задач из БД при запуске.        |

**Пример сигналов Qt**:
```cpp
connect(addButton, &QPushButton::clicked, this, &MainWindow::onAddButtonClicked);
connect(filterCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onFilterChanged);
```

---

## **Класс `TelegramNotifier`**
**Назначение**: Отправка уведомлений через Telegram (альтернатива `TelegramBot`).  
**Файлы**: `telegram_notifier.cpp`, `telegram_notifier.hpp`.

### **Ключевые поля**
- `bot_token` (string): Токен бота.  
- `chat_id` (string): Идентификатор чата для уведомлений.

### **Методы**
| Метод                          | Описание                                  |
|--------------------------------|-------------------------------------------|
| `send_message(text) -> bool`   | Отправляет сообщение. Возвращает `true` при успехе. |

**Пример**:
```cpp
TelegramNotifier notifier("TOKEN", "CHAT_ID");
notifier.send_message("Задача выполнена!");
```

---

## **Взаимодействие классов**
```plaintext
MainWindow -> DatabaseManager: saveTask(), getAllTasks()  
MainWindow -> TelegramBot: start(), stop()  
DatabaseManager <-> Task: Сериализация/десериализация  
TelegramBot -> ConfigManager: Чтение токена  
Task <-> PeriodicTracker: Управление периодичностью  
```

---

## **Логика работы**
### **Добавление задачи**
1. Пользователь заполняет поля в GUI.  
2. При нажатии "Сохранить" вызывается метод `MainWindow::onAddButtonClicked()`.  
3. Создается объект `Task`, валидируются данные.  
4. `DatabaseManager::saveTask()` сохраняет задачу в SQLite.  
5. Обновляется интерфейс (список задач и диаграмма).  

---

## **Зависимости и настройки**
- **Qt 6**: Установите через [Qt Installer](https://www.qt.io/download).  
- **SQLite3**:  
  ```bash
  # Ubuntu/Debian
  sudo apt install sqlite3 libsqlite3-dev
  ```
- **CMake**: Минимальная версия 3.10.  

---

## **Примеры работы с API**
### **Создание задачи программно**
```cpp
DatabaseManager db("tasks.db");
Task task("Задача", "Описание", Task::Type::OneTime);
db.saveTask(task);
```

### **Получение статистики**
```cpp
auto [completed, pending] = db.getTaskStats();
qDebug() << "Выполнено:" << completed << "Не выполнено:" << pending;
```

---

## **Обработка ошибок**
- **Некорректный дедлайн**:  
  ```cpp
  try {
      task.set_deadline(QDateTime()); // Пустая дата
  } catch (const std::invalid_argument& e) {
      qCritical() << "Ошибка:" << e.what();
  }
  ```
- **Ошибки БД**: Логируются в `logs`.

---

## **Советы по разработке**
1. **Стиль кода**:  
   - Имена классов в `CamelCase`.  
   - Имена методов в `snake_case`.  
2. **Коммиты**:  
   - Сообщения на английском.    

---

## **Вопросы и поддержка**
- **GitHub**: [GitHub](https://github.com/misis-programming2024-2025/misis2024f-24-17-kostionova-k-i/tree/final_project)  
- **Email**: m2402783@edu.misis.ru  
