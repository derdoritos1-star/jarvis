#include "ConfigManager.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

ConfigManager& ConfigManager::GetInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::LoadConfig(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filepath << std::endl;
            return false;
        }

        json j;
        file >> j;

        if (j.contains("api_key")) {
            api_key = j["api_key"].get<std::string>();
        }

        if (j.contains("steam_accounts")) {
            for (auto& [key, value] : j["steam_accounts"].items()) {
                SteamAccount acc;
                acc.username = value["username"].get<std::string>();
                acc.password = value["password"].get<std::string>();
                steam_accounts[std::stoi(key)] = acc;
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;
        return false;
    }
}

std::string ConfigManager::GetApiKey() const {
    return api_key;
}

SteamAccount ConfigManager::GetSteamAccount(int index) const {
    auto it = steam_accounts.find(index);
    if (it != steam_accounts.end()) {
        return it->second;
    }
    return {"", ""};
}
