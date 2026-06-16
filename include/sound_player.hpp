#pragma once

#include <cstdlib>
#include <filesystem>
#include <stdexcept>

#include "miniaudio.h"

class sound_player {
public:
  static sound_player &get_instance() {
    static sound_player soundPlayer;
    return soundPlayer;
  }

  bool play_sound(const std::filesystem::path &path) {
    return ma_engine_play_sound(&m_engine, path.string().c_str(), NULL) ==
           MA_SUCCESS;
  }

protected:
  sound_player() {
    if (ma_engine_init(NULL, &m_engine) != MA_SUCCESS) {
      throw std::runtime_error{"Could not initilize the maniaudio engine"};
    }
  }

  ~sound_player() { ma_engine_uninit(&m_engine); }

private:
  ma_engine m_engine;
};

class voice_player {
public:
  enum class Voice { Voice1, Voice2, Voice3, Voice4, Voice5, Voice6 };

  static voice_player &get_instance() {
    static voice_player voicePlayer;
    return voicePlayer;
  }

  bool play_voice(Voice voice) {
    std::filesystem::path path = TTS_VOICE_DIR / get_voice_filename(voice);
    return sound_player::get_instance().play_sound(path);
  }

protected:
  voice_player() = default;
  ~voice_player() = default;

  inline static std::filesystem::path TTS_VOICE_DIR = "tts_voice";
  inline static std::filesystem::path TTS_VOICE_FILE_1 = "1.mp3";
  inline static std::filesystem::path TTS_VOICE_FILE_2 = "2.mp3";
  inline static std::filesystem::path TTS_VOICE_FILE_3 = "3.mp3";
  inline static std::filesystem::path TTS_VOICE_FILE_4 = "4.mp3";
  inline static std::filesystem::path TTS_VOICE_FILE_5 = "5.mp3";
  inline static std::filesystem::path TTS_VOICE_FILE_6 = "6.mp3";

  std::filesystem::path get_voice_filename(Voice voice) {
    switch (voice) {
    case Voice::Voice1:
      return TTS_VOICE_FILE_1;
    case Voice::Voice2:
      return TTS_VOICE_FILE_2;
    case Voice::Voice3:
      return TTS_VOICE_FILE_3;
    case Voice::Voice4:
      return TTS_VOICE_FILE_4;
    case Voice::Voice5:
      return TTS_VOICE_FILE_5;
    case Voice::Voice6:
      return TTS_VOICE_FILE_6;
    default:
      throw std::invalid_argument{"Invalid voice"};
    }
  }
};
