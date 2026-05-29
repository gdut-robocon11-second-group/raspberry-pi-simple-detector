#include <iostream>

#include "sound_player.hpp"

int main() {
    SoundPlayer soundPlayer;
    soundPlayer.setLanguage("zh");
    soundPlayer.speak("Hello, Raspberry Pi Simple Detector!");
    return 0;
}
