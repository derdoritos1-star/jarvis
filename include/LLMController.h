#ifndef LLM_CONTROLLER_H
#define LLM_CONTROLLER_H

#include <string>

class LLMController {
public:
    static std::string SendPrompt(const std::string& prompt);
    static void ProcessResponse(const std::string& responseText);
};

#endif // LLM_CONTROLLER_H
