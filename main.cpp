#include <iostream>

#include "sound_player.hpp"

int main() {
    auto& soundPlayer = SoundPlayer::getInstance();
    soundPlayer.setLanguage("en");
    soundPlayer.speak("Hello, Raspberry Pi Simple Detector!");
    return 0;
}
