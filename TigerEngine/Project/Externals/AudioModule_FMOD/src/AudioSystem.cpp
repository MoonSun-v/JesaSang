#include "..\\include\\AudioSystem.h"
#include "..\\include\\AudioClip.h"
#include <Windows.h>
#include "..\\fmod\\inc\\fmod_errors.h"

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

    for (auto& entry : m_ChannelGroups) {
        if (entry.second) {
            entry.second->release();
        }
    }
    m_ChannelGroups.clear();
    m_MasterGroup = nullptr;

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

FMOD::ChannelGroup* AudioSystem::GetOrCreateChannelGroup(const std::string& name)
{
    if (!m_System) {
        return nullptr;
    }

    if (name.empty()) {
        return m_MasterGroup;
    }

    auto it = m_ChannelGroups.find(name);
    if (it != m_ChannelGroups.end()) {
        return it->second;
    }

    FMOD::ChannelGroup* group = nullptr;
    FMOD_RESULT result = m_System->createChannelGroup(name.c_str(), &group);
    if (result != FMOD_OK || !group) {
        return m_MasterGroup;
    }

    if (m_MasterGroup) {
        m_MasterGroup->addGroup(group);
    }
    m_ChannelGroups.emplace(name, group);
    return group;
}

void AudioSystem::SetChannelGroupVolume(const std::string& name, float volume)
{
    if (volume < 0.0f) {
        volume = 0.0f;
    }

    FMOD::ChannelGroup* group = GetOrCreateChannelGroup(name);
    if (group) {
        group->setVolume(volume);
    }
}

float AudioSystem::GetChannelGroupVolume(const std::string& name) const
{
    if (!m_System) {
        return m_MasterVolume;
    }

    if (name.empty()) {
        return m_MasterVolume;
    }

    auto it = m_ChannelGroups.find(name);
    if (it == m_ChannelGroups.end() || !it->second) {
        return m_MasterVolume;
    }

    float volume = m_MasterVolume;
    if (it->second->getVolume(&volume) == FMOD_OK) {
        return volume;
    }
    return m_MasterVolume;
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
