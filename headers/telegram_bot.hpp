#ifndef TELEGRAM_BOT_HPP
#define TELEGRAM_BOT_HPP

#include "config_manager.hpp"
#include "database_manager.hpp"
#include <curl/curl.h>
#include <string>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>
#include <QObject>
#include <QString>
#include <QTimer>

class TelegramBot : public QObject {
    Q_OBJECT

public:
    TelegramBot(const ConfigManager& config, DatabaseManager& db, QObject* parent = nullptr);
    ~TelegramBot();

    void start();
    void stop();

    bool send_message(const std::string& text, const std::string& chat_id = "") const;
    
    static bool send_direct_message(const std::string& bot_token, const std::string& chat_id, const std::string& text);

signals:
    void chatIdRegistered();

private:
    void pollingLoop();
    void processMessage(const std::string& text, const std::string& chat_id);
    void check_reminders();
    bool isUserRegistered(const std::string& chat_id) const;

    static size_t dummy_write_callback(void* buffer, size_t size, size_t nmemb, void* userp);
    static std::vector<std::string> split(const std::string& s, char delimiter);
    static std::string trim(const std::string& s);
    std::string getFirstChatId() const;

    std::string bot_token_;
    DatabaseManager& db_;
    bool running_;
    std::thread polling_thread_;
    QTimer reminderTimer;
};

#endif