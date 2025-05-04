#include "telegram_bot.hpp"
#include <curl/curl.h>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>
#include <cctype>

TelegramBot::TelegramBot(const std::string& bot_token, DatabaseManager& db)
    : bot_token_(bot_token), db_(db), running_(false) 
{}

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
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    if (!curl) {
        std::cerr << "CURL initialization failed." << std::endl;
        return;
    }
    
    long last_update_id = 0;
    std::string response;

    while (running_) {
        std::string url = "https://api.telegram.org/bot" + bot_token_ + "/getUpdates?timeout=10&offset=" + std::to_string(last_update_id + 1);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            size_t pos = response.find("\"text\":");
            if (pos != std::string::npos) {
                size_t start = response.find('"', pos + 7) + 1;
                size_t end = response.find('"', start);
                std::string text = response.substr(start, end - start);

                size_t chat_id_pos = response.find("\"chat\":{\"id\":");
                if (chat_id_pos != std::string::npos) {
                    size_t id_start = response.find(':', chat_id_pos) + 1;
                    std::string chat_id = response.substr(id_start, response.find(',', id_start) - id_start);
                    processMessage(text, chat_id);
                }
            }
            last_update_id++;
        }
        response.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    curl_easy_cleanup(curl);
}

static std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

static std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) start++;
    auto end = s.end();
    while (end != start && std::isspace(*(end - 1))) end--;
    return std::string(start, end);
}

void TelegramBot::processMessage(const std::string& text, const std::string& chat_id) {
    if (text.find("Добавить:") == 0) {
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