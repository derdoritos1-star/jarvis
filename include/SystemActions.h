#ifndef SYSTEM_ACTIONS_H
#define SYSTEM_ACTIONS_H

#include <string>

class SystemActions {
public:
    static bool OpenApp(const std::string& appName);
    static bool SwitchSteamAccount(int accountIndex);
    static void PressShortcut(const std::string& shortcutType);

private:
    static std::string SearchSteamGames(const std::string& appName);
    static std::string SearchRegistry(const std::string& appName);
    static std::string SearchShortcuts(const std::string& appName);
    static std::string FindSteamPath();
};

#endif // SYSTEM_ACTIONS_H
