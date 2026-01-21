#pragma once

#include "..\\fmod\\inc\\fmod.hpp"
#include <memory>
#include <string>

class AudioClip;

class AudioSystem {
public:
    bool Init();
    void Shutdown();
    void Update();

    void Set3DSettings(float dopplerScale, float distanceFactor, float rolloffScale);
    void SetMasterVolume(float volume);
    float GetMasterVolume() const { return m_MasterVolume; }

    FMOD::System* GetSystem() const { return m_System; }
    std::shared_ptr<AudioClip> CreateClip(const std::string& path, FMOD_MODE mode);

private:
    FMOD::System* m_System = nullptr;
    FMOD::ChannelGroup* m_MasterGroup = nullptr;
    float m_MasterVolume = 1.0f;
};
