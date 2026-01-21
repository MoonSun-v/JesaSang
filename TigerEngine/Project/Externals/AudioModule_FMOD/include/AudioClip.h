#pragma once

#include "..\\fmod\\inc\\fmod.hpp"
#include <string>

class AudioClip {
public:
    AudioClip(FMOD::System* system, const std::string& path, FMOD_MODE mode);
    ~AudioClip();

    bool IsValid() const { return m_Sound != nullptr; }
    FMOD::Sound* GetSound() const { return m_Sound; }
    float GetLengthSeconds() const { return m_LengthSeconds; }

private:
    FMOD::Sound* m_Sound = nullptr;
    float m_LengthSeconds = 0.0f;
};
