#include "AudioSystem.h"
#include "AudioClip.h"
#include <Windows.h>
#include <fmod_errors.h>

bool AudioSystem::Init()
{
    if (m_System) {
        return true;
    }

    FMOD_RESULT result = FMOD::System_Create(&m_System);
    if (result != FMOD_OK || !m_System) {
        OutputDebugStringA("[AudioSystem] FMOD System_Create failed: ");
        OutputDebugStringA(FMOD_ErrorString(result));
        OutputDebugStringA("\n");
        m_System = nullptr;
        return false;
    }

    result = m_System->init(512, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) {
        OutputDebugStringA("[AudioSystem] FMOD init failed: ");
        OutputDebugStringA(FMOD_ErrorString(result));
        OutputDebugStringA("\n");
        m_System->release();
        m_System = nullptr;
        return false;
    }

    m_System->getMasterChannelGroup(&m_MasterGroup);
    if (m_MasterGroup) {
        m_MasterGroup->setVolume(m_MasterVolume);
    }

    return true;
}

void AudioSystem::Shutdown()
{
    if (!m_System) {
        return;
    }

    m_System->close();
    m_System->release();
    m_System = nullptr;
}

void AudioSystem::Update()
{
    if (m_System) {
        m_System->update();
    }
}

void AudioSystem::Set3DSettings(float dopplerScale, float distanceFactor, float rolloffScale)
{
    if (!m_System) {
        return;
    }
    m_System->set3DSettings(dopplerScale, distanceFactor, rolloffScale);
}

void AudioSystem::SetMasterVolume(float volume)
{
    m_MasterVolume = volume;
    if (m_MasterGroup) {
        m_MasterGroup->setVolume(volume);
    }
}

std::shared_ptr<AudioClip> AudioSystem::CreateClip(const std::string& path, FMOD_MODE mode)
{
    if (!m_System) {
        return {};
    }
    auto clip = std::make_shared<AudioClip>(m_System, path, mode);
    if (!clip->IsValid()) {
        return {};
    }
    return clip;
}
