#include "SystemActions.h"
#include "ConfigManager.h"
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <shlwapi.h>
#include <shlobj.h>
#include <fstream>
#include <algorithm>
#include <regex>

void KillProcessByName(const char* filename) {
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    PROCESSENTRY32 pEntry;
    pEntry.dwSize = sizeof(pEntry);
    BOOL hRes = Process32First(hSnapShot, &pEntry);
    while (hRes) {
        if (strcmp(pEntry.szExeFile, filename) == 0) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0,
                                          (DWORD)pEntry.th32ProcessID);
            if (hProcess != NULL) {
                TerminateProcess(hProcess, 9);
                CloseHandle(hProcess);
            }
        }
        hRes = Process32Next(hSnapShot, &pEntry);
    }
    CloseHandle(hSnapShot);
}

std::string SystemActions::FindSteamPath() {
    HKEY hKey;
    char buffer[MAX_PATH];
    DWORD bufferSize = sizeof(buffer);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "SteamExe", NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(buffer);
        }
        RegCloseKey(hKey);
    }
    return "";
}

std::string SystemActions::SearchSteamGames(const std::string& appName) {
    std::string lowerAppName = appName;
    std::transform(lowerAppName.begin(), lowerAppName.end(), lowerAppName.begin(), ::tolower);

    // Hardcode some common aliases
    if (lowerAppName == "кс2" || lowerAppName == "cs2" || lowerAppName == "counter-strike 2") return "730";
    if (lowerAppName == "дота" || lowerAppName == "dota 2" || lowerAppName == "dota") return "570";

    std::string steamPath = FindSteamPath();
    if (steamPath.empty()) return "";

    std::string steamDir = steamPath.substr(0, steamPath.find_last_of("\\/"));
    std::string vdfPath = steamDir + "\\steamapps\\libraryfolders.vdf";

    std::ifstream file(vdfPath);
    if (!file.is_open()) return "";

    std::string line;
    std::string currentAppId = "";
    std::regex appIdRegex("\"(\\\\d+)\"");
    std::regex nameRegex("\"name\"\\\\s+\"([^\"]+)\"");
    std::smatch match;

    // A very basic VDF parser. In reality, we'd look in steamapps/appmanifest_*.acf
    // but looking for aliases is more reliable for voice commands.
    return "";
}

std::string SystemActions::SearchRegistry(const std::string& appName) {
    HKEY hKey;
    std::string result = "";
    std::string lowerAppName = appName;
    std::transform(lowerAppName.begin(), lowerAppName.end(), lowerAppName.begin(), ::tolower);

    std::string subKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\" + lowerAppName + ".exe";
    char buffer[MAX_PATH];
    DWORD bufferSize = sizeof(buffer);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, NULL, NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            result = buffer;
        }
        RegCloseKey(hKey);
        if (!result.empty()) return result;
    }

    if (RegOpenKeyExA(HKEY_CURRENT_USER, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, NULL, NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            result = buffer;
        }
        RegCloseKey(hKey);
        if (!result.empty()) return result;
    }

    return "";
}

std::string SystemActions::SearchShortcuts(const std::string& appName) {
    std::string lowerAppName = appName;
    std::transform(lowerAppName.begin(), lowerAppName.end(), lowerAppName.begin(), ::tolower);

    char path[MAX_PATH];
    std::vector<std::string> searchDirs;

    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, path))) searchDirs.push_back(path);
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROGRAMS, NULL, 0, path))) searchDirs.push_back(path);
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path))) searchDirs.push_back(path);
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, path))) searchDirs.push_back(path);

    IShellLinkA* psl = NULL;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkA, (LPVOID*)&psl))) return "";

    IPersistFile* ppf = NULL;
    if (FAILED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf))) {
        psl->Release();
        return "";
    }

    std::string foundPath = "";

    for (const auto& dir : searchDirs) {
        std::string searchPath = dir + "\\*.lnk";
        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::string filename = fd.cFileName;
                std::string lowerFilename = filename;
                std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);

                if (lowerFilename.find(lowerAppName) != std::string::npos) {
                    std::string fullPath = dir + "\\" + filename;
                    WCHAR wsz[MAX_PATH];
                    MultiByteToWideChar(CP_ACP, 0, fullPath.c_str(), -1, wsz, MAX_PATH);

                    if (SUCCEEDED(ppf->Load(wsz, STGM_READ))) {
                        char targetPath[MAX_PATH];
                        if (SUCCEEDED(psl->GetPath(targetPath, MAX_PATH, NULL, SLGP_UNCPRIORITY))) {
                            foundPath = targetPath;
                            break;
                        }
                    }
                }
            } while (FindNextFileA(hFind, &fd) && foundPath.empty());
            FindClose(hFind);
        }
        if (!foundPath.empty()) break;
    }

    ppf->Release();
    psl->Release();
    return foundPath;
}

bool SystemActions::OpenApp(const std::string& appName) {
    std::cout << "Attempting to open: " << appName << std::endl;

    // 1. Steam check
    std::string appId = SearchSteamGames(appName);
    if (!appId.empty()) {
        std::string uri = "steam://rungameid/" + appId;
        ShellExecuteA(NULL, "open", uri.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return true;
    }

    // 2. Registry check
    std::string exePath = SearchRegistry(appName);
    if (!exePath.empty()) {
        ShellExecuteA(NULL, "open", exePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return true;
    }

    // 3. Shortcuts check
    exePath = SearchShortcuts(appName);
    if (!exePath.empty()) {
        ShellExecuteA(NULL, "open", exePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return true;
    }

    std::cerr << "Could not find application: " << appName << std::endl;
    return false;
}

bool SystemActions::SwitchSteamAccount(int accountIndex) {
    SteamAccount acc = ConfigManager::GetInstance().GetSteamAccount(accountIndex);
    if (acc.username.empty()) {
        std::cerr << "Steam account not found for index: " << accountIndex << std::endl;
        return false;
    }

    std::string steamPath = FindSteamPath();
    if (steamPath.empty()) {
        std::cerr << "Steam not found in registry" << std::endl;
        return false;
    }

    KillProcessByName("steam.exe");
    Sleep(2000);

    std::string params = "-login " + acc.username + " " + acc.password;

    HINSTANCE result = ShellExecuteA(NULL, "open", steamPath.c_str(), params.c_str(), NULL, SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32) {
        std::cerr << "Failed to start steam" << std::endl;
        return false;
    }

    return true;
}

void SystemActions::PressShortcut(const std::string& shortcutType) {
    if (shortcutType == "win_r") {
        INPUT inputs[4] = {};

        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_LWIN;

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = 'R';

        inputs[2].type = INPUT_KEYBOARD;
        inputs[2].ki.wVk = 'R';
        inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

        inputs[3].type = INPUT_KEYBOARD;
        inputs[3].ki.wVk = VK_LWIN;
        inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(4, inputs, sizeof(INPUT));
    } else if (shortcutType == "alt_tab") {
        INPUT inputs[4] = {};

        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_MENU; // ALT

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = VK_TAB;

        inputs[2].type = INPUT_KEYBOARD;
        inputs[2].ki.wVk = VK_TAB;
        inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

        inputs[3].type = INPUT_KEYBOARD;
        inputs[3].ki.wVk = VK_MENU;
        inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(4, inputs, sizeof(INPUT));
    }
}
