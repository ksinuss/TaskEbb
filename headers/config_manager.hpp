#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <string>
#include <stdexcept>

class ConfigManager {
public:
    ConfigManager(const std::string& config_path = "config/config.ini");
    
    std::string get_bot_token() const;
    std::string get_db_path() const;

private:
    std::string config_path_;
    std::string read_key(const std::string& section, const std::string& key) const;
};

#endif