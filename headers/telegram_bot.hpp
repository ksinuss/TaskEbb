#ifndef TELEGRAM_BOT_HPP
#define TELEGRAM_BOT_HPP

#include <string>
#include <thread>
#include <atomic>
#include "task.hpp"
#include "database_manager.hpp"

class TelegramBot {
public:
    TelegramBot(const std::string& bot_token, DatabaseManager& db);
    
    ~TelegramBot();

    void start();

    void stop();

private:
    void pollingLoop();

    void processMessage(const std::string& text, const std::string& chat_id);

    void send_message(const std::string& text, const std::string& chat_id) const;

    std::string bot_token_;
    DatabaseManager& db_;
    std::atomic<bool> running_;
    std::thread polling_thread_;
};

#endif