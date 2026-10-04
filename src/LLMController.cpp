#include "LLMController.h"
#include "ConfigManager.h"
#include "SystemActions.h"
#include "AudioControl.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string LLMController::SendPrompt(const std::string& prompt) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        std::string apiKey = ConfigManager::GetInstance().GetApiKey();

        // Example for OpenAI API (can be swapped to Gemini, etc.)
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string authHeader = "Authorization: Bearer " + apiKey;
        headers = curl_slist_append(headers, authHeader.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Tell the model we want function calling JSON format if it tries to execute an action
        json payload = {
            {"model", "gpt-3.5-turbo"},
            {"messages", json::array({
                {
                    {"role", "system"},
                    {"content", "You are an AI assistant named Doritos. If the user asks you to perform an action (e.g. open an app, switch steam account), return ONLY a valid JSON object describing the action. Do not wrap it in markdown block. Example: {\"action\": \"search_and_open\", \"app_name\": \"cs2\"} or {\"action\": \"switch_steam\", \"account\": 2}. Otherwise, respond conversationally."}
                },
                {
                    {"role", "user"},
                    {"content", prompt}
                }
            })}
        };

        std::string payloadStr = payload.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Skip SSL verification if needed on local windows tests without CA certs, though we enabled Schannel
        // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return readBuffer;
}

void LLMController::ProcessResponse(const std::string& responseText) {
    if (responseText.empty()) return;

    try {
        json j = json::parse(responseText);
        if (j.contains("choices") && j["choices"].is_array() && j["choices"].size() > 0) {
            std::string content = j["choices"][0]["message"]["content"].get<std::string>();

            // Check if the content is a JSON object (action)
            if (content.front() == '{' && content.back() == '}') {
                try {
                    json actionJson = json::parse(content);
                    if (actionJson.contains("action")) {
                        std::string action = actionJson["action"].get<std::string>();
                        if (action == "search_and_open" && actionJson.contains("app_name")) {
                            SystemActions::OpenApp(actionJson["app_name"].get<std::string>());
                        } else if (action == "switch_steam" && actionJson.contains("account")) {
                            SystemActions::SwitchSteamAccount(actionJson["account"].get<int>());
                        } else {
                            std::cout << "Unknown action: " << content << std::endl;
                        }
                    }
                } catch(...) {
                    // Not valid JSON, treat as speech
                    AudioControl::Speak(content);
                }
            } else {
                // Regular speech
                std::cout << "AI: " << content << std::endl;
                AudioControl::Speak(content);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error parsing LLM response: " << e.what() << std::endl;
    }
}
