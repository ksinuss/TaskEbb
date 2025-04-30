#include "telegram_notifier.hpp"
#include <curl/curl.h>

TelegramNotifier::TelegramNotifier(const std::string& bot_token, const std::string& chat_id)
    : bot_token_(bot_token), chat_id_(chat_id) {}

TelegramNotifier::~TelegramNotifier() {}

bool TelegramNotifier::send_message(const std::string& text) const {
    CURL* curl = curl_easy_init();
    if (!curl || text.empty()) return false;

    std::string url = "https://api.telegram.org/bot" + bot_token_ + "/sendMessage";
    
    char* encoded_text = curl_easy_escape(curl, text.c_str(), text.length());
    char* encoded_chat_id = curl_easy_escape(curl, chat_id_.c_str(), chat_id_.length());
    
    std::string params = "chat_id=" +  std::string(encoded_chat_id) + "&text=" + std::string(encoded_text);

    curl_free(encoded_text);
    curl_free(encoded_chat_id);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return (res == CURLE_OK);
}

size_t TelegramNotifier::write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    return size * nmemb;
}