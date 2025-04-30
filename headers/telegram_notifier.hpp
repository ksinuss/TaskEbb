#ifndef TELEGRAM_NOTIFIER_HPP
#define TELEGRAM_NOTIFIER_HPP

#include <string>
#include <curl/curl.h>

class TelegramNotifier {
public:
    /**
     * @brief Конструктор класса для отправки уведомлений через Telegram.
     * @param bot_token Токен бота, полученный от @BotFather.
     * @param chat_id ID чата, куда отправляются уведомления.
     */
    TelegramNotifier(const std::string& bot_token, const std::string& chat_id);
    
    ~TelegramNotifier();

    /**
     * @brief Отправляет сообщение в Telegram.
     * @param text Текст сообщения.
     * @return true, если сообщение отправлено успешно, иначе false.
     */
    bool send_message(const std::string& text) const;

private:
    std::string bot_token_;
    std::string chat_id_;
    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
};

#endif