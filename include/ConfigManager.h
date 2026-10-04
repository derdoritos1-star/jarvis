#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>

struct SteamAccount {
    std::string username;
    std::string password;
};

class ConfigManager {
public:
    static ConfigManager& GetInstance();

    bool LoadConfig(const std::string& filepath);

    std::string GetApiKey() const;
    SteamAccount GetSteamAccount(int index) const;

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    std::string api_key;
    std::map<int, SteamAccount> steam_accounts;
};

#endif // CONFIG_MANAGER_H
