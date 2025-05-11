#include "telegram_bot.hpp"
#include <curl/curl.h>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>
#include <cctype>
#include <codecvt>
#include <nlohmann/json.hpp>

TelegramBot::TelegramBot(const ConfigManager& config, DatabaseManager& db, QObject* parent)
    : QObject(parent), bot_token_(config.get_bot_token()), db_(db), running_(false) 
{
    if (bot_token_.empty()) {
        throw std::invalid_argument("Bot token is not configured!");
    }
    
    reminderTimer.setInterval(60000); 
    connect(&reminderTimer, &QTimer::timeout, this, &TelegramBot::check_reminders);
    reminderTimer.start();
}

TelegramBot::~TelegramBot() {
    stop();
}

void TelegramBot::start() {
    running_ = true;
    polling_thread_ = std::thread(&TelegramBot::pollingLoop, this);
}

void TelegramBot::stop() {
    running_ = false;
    if (polling_thread_.joinable()) {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(500)) {
            if (!polling_thread_.joinable()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (polling_thread_.joinable()) {
            polling_thread_.detach();
        }
    }
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append(static_cast<char*>(contents), total_size);
    return total_size;
}

void TelegramBot::pollingLoop() {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[ERROR] Failed to initialize CURL" << std::endl;
        return;
    }

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 25L); ///< timeout is 25 seconds
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "TaskEbbBot/1.0");
    std::string response;
    long last_update_id = 0;

    while (running_) {
        std::string url = "https://api.telegram.org/bot" + bot_token_ + 
                        "/getUpdates?timeout=20&offset=" + 
                        std::to_string(last_update_id + 1);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "[ERROR] CURL request failed: " 
                    << curl_easy_strerror(res) << std::endl;
            response.clear();
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        ///< json parsing
        try {
            nlohmann::json json_response = nlohmann::json::parse(response);
            
            if (json_response["ok"] == true) {
                for (const auto& update : json_response["result"]) {
                    last_update_id = update["update_id"].get<long>();
                    
                    if (update.contains("message") && 
                        update["message"].contains("text")) {
                        
                        const auto& message = update["message"];
                        std::string chat_id = std::to_string(
                            message["chat"]["id"].get<int64_t>()
                        );
                        std::string text = message["text"].get<std::string>();
                        
                        QMetaObject::invokeMethod(this, 
                            [this, text, chat_id]() {
                                processMessage(text, chat_id);
                            },
                            Qt::QueuedConnection
                        );
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] JSON parsing failed: " << e.what() 
                    << "\nResponse: " << response << std::endl;
        }

        response.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    curl_easy_cleanup(curl);
    std::cout << "Polling loop stopped" << std::endl;
}

std::vector<std::string> TelegramBot::split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string TelegramBot::trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) start++;
    auto end = s.end();
    while (end != start && std::isspace(*(end - 1))) end--;
    return std::string(start, end);
}

void TelegramBot::processMessage(const std::string& text, const std::string& chat_id) {
    std::cout << "[DEBUG] Обработка сообщения: " << text << " от chat_id: " << chat_id << std::endl;
    
    if (text.find("/start") == 0) {
        try {
            db_.saveChatId(chat_id);
            std::cout << "[INFO] Chat ID сохранен: " << chat_id << std::endl;
            send_message("✅ Аккаунт привязан!", chat_id);
            emit chatIdRegistered(); ///< signal for GUI
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Ошибка сохранения chat_id: " << e.what() << std::endl;
            send_message("❌ Ошибка привязки: " + std::string(e.what()), chat_id);
        }
    }
    
    if (text.find("/add_task") == 0) {
        if (!isUserRegistered(chat_id)) {
            send_message("❌ Сначала выполните /start", chat_id);
            return;
        }
        std::string content = text.substr(9);
        size_t comma_pos = content.find(',');
        std::string title = (comma_pos != std::string::npos) ? content.substr(0, comma_pos) : content;
        std::string description = (comma_pos != std::string::npos) ? content.substr(comma_pos + 1) : "";
        try {
            Task task(title, description);
            db_.saveTask(task, "tasks", [](sqlite3_stmt* stmt, const Task& t) {
                sqlite3_bind_text(stmt, 1, t.get_id().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 2, t.get_title().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(stmt, 3, t.get_description().c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt, 4, t.is_completed() ? 1 : 0);
                sqlite3_bind_int(stmt, 5, t.get_interval().count());
            });
            send_message("✅ Задача добавлена: " + title, chat_id);
        } catch (const std::exception& e) {
            send_message("❌ Ошибка: " + std::string(e.what()), chat_id);
        }
    }

    if (text.find("/add_template") == 0) {
        if (!isUserRegistered(chat_id)) {
            send_message("❌ Сначала выполните /start", chat_id);
            return;
        }
        std::vector<std::string> parts = split(text, ',');
        if (parts.size() < 3) {
            send_message("❌ Формат: /add_template Название, Описание, Интервал", chat_id);
            return;
        }
        try {
            std::string commandPrefix = "/add_template ";
            std::string titlePart = parts[0];
            size_t prefixPos = titlePart.find(commandPrefix);
            if (prefixPos != std::string::npos) {
                titlePart = titlePart.substr(prefixPos + commandPrefix.length());
            }
            std::string title = trim(titlePart);
            std::string description = trim(parts[1]);
            int interval = std::stoi(trim(parts[2]));
            TaskTemplate tmpl(title, description, interval);
            db_.saveTemplate(tmpl);
            send_message("✅ Шаблон создан: " + title, chat_id);
        } catch (...) {
            send_message("❌ Ошибка при создании шаблона", chat_id);
        }
    }

    if (text.find("/complete_task") == 0) {
        if (!isUserRegistered(chat_id)) {
            send_message("❌ Сначала выполните /start", chat_id);
            return;
        }
        std::string task_id = text.substr(14);
        try {
            auto tasks = db_.getAllTasks("tasks", [](sqlite3_stmt* stmt) {
                Task task(
                    reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
                    reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))
                );
                task.set_id(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                task.mark_completed(sqlite3_column_int(stmt, 3) == 1);
                task.set_interval(std::chrono::hours(sqlite3_column_int(stmt, 4)));
                return task;
            });
            for (auto& task : tasks) {
                if (task.get_id() == task_id) {
                    task.mark_completed(true);
                    task.mark_execution(std::chrono::system_clock::now());
                    db_.updateTask(task, "tasks", [](sqlite3_stmt* stmt, const Task& t) {
                        sqlite3_bind_text(stmt, 1, t.get_title().c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(stmt, 2, t.get_description().c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_int(stmt, 3, t.is_completed() ? 1 : 0);
                        sqlite3_bind_int(stmt, 4, t.get_interval().count());
                        sqlite3_bind_text(stmt, 5, t.get_id().c_str(), -1, SQLITE_TRANSIENT);
                    });
                    send_message("✅ Задача выполнена!", chat_id);
                    break;
                }
            }
        } catch (...) {
            send_message("❌ Ошибка при выполнении задачи", chat_id);
        }
    }
}

void TelegramBot::send_message(const std::string& text, const std::string& chat_id) const {
    if (text.empty()) return;

    CURL* curl = curl_easy_init();
    std::string url = "https://api.telegram.org/bot" + bot_token_ + "/sendMessage";
    
    char* encoded_text = curl_easy_escape(curl, text.c_str(), text.length());
    char* encoded_chat_id = curl_easy_escape(curl, chat_id.c_str(), chat_id.length());
    
    std::string params = "chat_id=" + std::string(encoded_chat_id) + 
                         "&text=" + std::string(encoded_text);
    
    curl_free(encoded_text);
    curl_free(encoded_chat_id);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L); 
    
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
}

bool TelegramBot::isUserRegistered(const std::string& chat_id) const {
    auto chats = db_.getAllChatIds();
    return std::find(chats.begin(), chats.end(), chat_id) != chats.end();
}

void TelegramBot::check_reminders() {
    auto chatIds = db_.getAllChatIds();

    for (const auto& chat_id : chatIds) {
        auto tasks = db_.getAllTasks("tasks", [](sqlite3_stmt* stmt) {
            Task task(
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)), // title
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))   // description
            );
            task.set_id(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))); // id
            task.mark_completed(sqlite3_column_int(stmt, 3) == 1); // is_completed
            task.set_interval(std::chrono::hours(sqlite3_column_int(stmt, 4))); // interval
            
            time_t first_exec = sqlite3_column_int64(stmt, 5);
            time_t second_exec = sqlite3_column_int64(stmt, 6);
            if (first_exec > 0) {
                task.mark_execution(
                    std::chrono::system_clock::from_time_t(first_exec)
                );
            }
            if (second_exec > 0) {
                task.mark_execution(
                    std::chrono::system_clock::from_time_t(second_exec)
                );
            }
            return task;
        });

        for (auto task : tasks) {
            if (auto next_time = task.get_tracker().get_next_execution_time()) {
                if (std::chrono::system_clock::now() >= *next_time) {
                    send_message("⏰ Напоминание: " + task.get_title(), chat_id);
                    task.mark_execution(std::chrono::system_clock::now());
                    db_.updateTask(task, "tasks", [](sqlite3_stmt* stmt, const Task& t) {
                        sqlite3_bind_text(stmt, 1, t.get_title().c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_text(stmt, 2, t.get_description().c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_bind_int(stmt, 3, t.is_completed() ? 1 : 0);
                        sqlite3_bind_int(stmt, 4, t.get_interval().count());
                        sqlite3_bind_text(stmt, 5, t.get_id().c_str(), -1, SQLITE_TRANSIENT);
                    });
                }
            }
        }
    }
}
