#include "config_manager.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

ConfigManager::ConfigManager(const std::string& config_path) : config_path_("config/config.ini")
{
    if (!std::filesystem::exists(config_path_)) {
        throw std::runtime_error("Создайте config.ini в папке config/");
    }
}

std::string ConfigManager::get_bot_token() const {
    return read_key("Telegram", "BotToken");
}

std::string ConfigManager::get_db_path() const {
    return read_key("Database", "Path");
}

std::string ConfigManager::read_key(const std::string& section, const std::string& key) const {
    std::ifstream file(config_path_);
    if (!file.is_open()) {
        throw std::runtime_error("Config file not found: " + config_path_);
    }
    
    std::string current_section;
    std::string line;
    
    while (std::getline(file, line)) {
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
        if (line.empty()) continue;
        
        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            continue;
        }
        
        if (current_section == section) {
            size_t delimiter = line.find('=');
            if (delimiter != std::string::npos) {
                std::string k = line.substr(0, delimiter);
                if (k == key) {
                    return line.substr(delimiter + 1);
                }
            }
        }
    }
    
    throw std::runtime_error("Key '" + key + "' not found in section [" + section + "]");
}