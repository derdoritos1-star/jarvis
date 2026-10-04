#include "AudioControl.h"
#include "LLMController.h"
#include <windows.h>
#include <sapi.h>
#include <sphelper.h>

#include <iostream>
#include <thread>
#include <algorithm>
#include <codecvt>
#include <locale>

ISpVoice* pVoice = nullptr;
bool AudioControl::isListening = false;
std::thread listenThread;

std::wstring Utf8ToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string WStringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

bool AudioControl::Initialize() {
    if (FAILED(::CoInitialize(NULL))) return false;

    HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize SAPI Voice" << std::endl;
        return false;
    }
    return true;
}

void AudioControl::Cleanup() {
    if (pVoice) {
        pVoice->Release();
        pVoice = nullptr;
    }
    ::CoUninitialize();
}

void AudioControl::Speak(const std::string& text) {
    if (pVoice) {
        std::wstring wtext = Utf8ToWString(text);
        pVoice->Speak(wtext.c_str(), 0, NULL);
    }
}

void AudioControl::StartListening() {
    isListening = true;
    listenThread = std::thread(ListeningThread);
}

void AudioControl::StopListening() {
    isListening = false;
    if (listenThread.joinable()) {
        listenThread.join();
    }
}

void AudioControl::ListeningThread() {
    if (FAILED(::CoInitialize(NULL))) return;

    ISpRecognizer* cpEngine = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SpSharedRecognizer, NULL, CLSCTX_ALL, IID_ISpRecognizer, (void**)&cpEngine);
    if (FAILED(hr)) {
        std::cerr << "Failed to create SAPI Recognizer" << std::endl;
        ::CoUninitialize();
        return;
    }

    ISpRecoContext* cpRecoCtx = nullptr;
    hr = cpEngine->CreateRecoContext(&cpRecoCtx);
    if (FAILED(hr)) {
        std::cerr << "Failed to create SAPI Reco Context" << std::endl;
        if (cpEngine) cpEngine->Release();
        ::CoUninitialize();
        return;
    }

    hr = cpRecoCtx->SetNotifyWin32Event();
    HANDLE hEvent = cpRecoCtx->GetNotifyEventHandle();

    ULONGLONG ullInterest = SPFEI(SPEI_RECOGNITION);
    hr = cpRecoCtx->SetInterest(ullInterest, ullInterest);

    ISpRecoGrammar* cpGrammar = nullptr;
    hr = cpRecoCtx->CreateGrammar(0, &cpGrammar);
    if (FAILED(hr)) {
        std::cerr << "Failed to create SAPI Grammar" << std::endl;
        if (cpRecoCtx) cpRecoCtx->Release();
        if (cpEngine) cpEngine->Release();
        ::CoUninitialize();
        return;
    }

    hr = cpGrammar->LoadDictation(NULL, SPLO_STATIC);
    hr = cpGrammar->SetDictationState(SPRS_ACTIVE);

    std::cout << "Jarvis is now listening..." << std::endl;

    while (isListening) {
        DWORD dwWait = WaitForSingleObject(hEvent, 1000); // 1 sec timeout
        if (dwWait == WAIT_OBJECT_0) {
            SPEVENT event;
            while (cpRecoCtx->GetEvents(1, &event, NULL) == S_OK) {
                if (event.eEventId == SPEI_RECOGNITION) {
                    ISpRecoResult* pResult = (ISpRecoResult*)event.lParam;
                    WCHAR* pwszText = NULL;
                    if (SUCCEEDED(pResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &pwszText, NULL))) {
                        std::string recognizedStr = WStringToUtf8(pwszText);
                        std::cout << "Recognized: " << recognizedStr << std::endl;

                        // Convert to lowercase for wake word check
                        std::string lowerStr = recognizedStr;
                        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);

                        if (lowerStr.find("doritos") != std::string::npos || lowerStr.find("доритос") != std::string::npos) {
                            std::cout << "Wake word detected!" << std::endl;
                            // Send to LLM
                            std::string llmResponse = LLMController::SendPrompt(recognizedStr);
                            LLMController::ProcessResponse(llmResponse);
                        } else {
                            std::cout << "Ignored (no wake word)" << std::endl;
                        }

                        ::CoTaskMemFree(pwszText);
                    }
                }
            }
        }
    }

    if (cpGrammar) cpGrammar->Release();
    if (cpRecoCtx) cpRecoCtx->Release();
    if (cpEngine) cpEngine->Release();
    ::CoUninitialize();
}
