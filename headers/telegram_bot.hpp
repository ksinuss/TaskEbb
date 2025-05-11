#ifndef TELEGRAM_BOT_HPP
#define TELEGRAM_BOT_HPP

#include <QObject>
#include <QString>
#include <QTimer>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include "config_manager.hpp"
#include "database_manager.hpp"

class TelegramBot : public QObject {
    Q_OBJECT

public:
    explicit TelegramBot(const ConfigManager& config, DatabaseManager& db, QObject* parent = nullptr);
    ~TelegramBot();

    void start();
    void stop();

signals:
    void chatIdRegistered();

public slots:
    void send_message(const std::string& text, const std::string& chat_id) const;
    
private:
    void pollingLoop();
    void processMessage(const std::string& text, const std::string& chat_id);
    bool isUserRegistered(const std::string& chat_id) const;

    static std::vector<std::string> split(const std::string& s, char delimiter);
    static std::string trim(const std::string& s);

    std::string bot_token_;
    DatabaseManager& db_;
    bool running_;
    std::thread polling_thread_;

    QTimer reminderTimer;
    void check_reminders();
};

#endif