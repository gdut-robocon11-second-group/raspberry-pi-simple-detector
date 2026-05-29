#pragma once

#include <string_view>
#include <string>
#include <filesystem>
#include <cstdlib>

class SoundPlayer {
public:
    SoundPlayer() = default;
    ~SoundPlayer() = default;

    void speak(std::string_view text) {
        
    }

    void setLanguage(std::string_view text) {
        m_lang = std::string(text);
    }

private:
    std::string m_lang{"zh"};
};
