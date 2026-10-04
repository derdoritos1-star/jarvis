#ifndef AUDIO_CONTROL_H
#define AUDIO_CONTROL_H

#include <string>

class AudioControl {
public:
    static bool Initialize();
    static void Cleanup();
    static void Speak(const std::string& text);
    static void StartListening(); // Runs in a background thread
    static void StopListening();

private:
    static bool isListening;
    static void ListeningThread();
};

#endif // AUDIO_CONTROL_H
