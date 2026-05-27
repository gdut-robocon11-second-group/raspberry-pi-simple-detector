#pragma once

#include <string_view>
#include <filesystem>

#include <espeak-ng/speak_lib.h>

class SoundPlayer {
public:
    SoundPlayer(const SoundPlayer&) = delete;
    SoundPlayer& operator=(const SoundPlayer&) = delete;
    SoundPlayer(SoundPlayer&&) = delete;
    SoundPlayer& operator=(SoundPlayer&&) = delete;

    static SoundPlayer& getInstance() {
        static SoundPlayer instance;
        return instance;
    }

    void setLanguage(std::string_view language) {
        espeak_SetVoiceByName(language.data());
    }

    void speak(std::string_view text) {
        espeak_Synth(text.data(), text.size(), 0, POS_CHARACTER, 0, espeakCHARS_AUTO, nullptr, nullptr);
        espeak_Synchronize();
    }

    void asyncSpeak(std::string_view text) {
        espeak_Synth(text.data(), text.size(), 0, POS_CHARACTER, 0, espeakCHARS_AUTO, nullptr, nullptr);
    }

protected:
    SoundPlayer() {
        espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 0, nullptr, 0);
    }

    ~SoundPlayer() {
        espeak_Terminate();
    }
};
