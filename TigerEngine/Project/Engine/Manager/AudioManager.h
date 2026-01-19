#pragma once

#include <pch.h>
#include <System/Singleton.h>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "AudioClip.h"
#include "AudioSystem.h"

struct AudioManifestEntry
{
    std::string id;
    std::string path;
    std::string group;
    FMOD_MODE mode = FMOD_DEFAULT;
    float defaultVolume = 1.0f;
    bool loop = false;
};

class AudioManager : public Singleton<AudioManager>
{
public:
    AudioManager(token) {}
    ~AudioManager() = default;

    bool Initialize(const std::string& manifestPath = "");
    void Shutdown();
    void Update();

    bool LoadManifest(const std::string& manifestPath = "");
    const AudioManifestEntry* GetEntry(const std::string& id) const;
    std::shared_ptr<AudioClip> GetOrCreateClip(const std::string& id);
    void PreloadAll();

    AudioSystem& GetSystem() { return m_system; }

private:
    std::filesystem::path ResolveManifestPath(const std::string& manifestPath) const;

    static std::string Trim(std::string value);
    static std::vector<std::string> SplitCsvLine(const std::string& line);
    static FMOD_MODE ParseMode(const std::string& text);

    std::unordered_map<std::string, AudioManifestEntry> m_entries;
    std::unordered_map<std::string, std::shared_ptr<AudioClip>> m_clips;
    AudioSystem m_system;
    std::filesystem::path m_manifestPath;
    bool m_initialized = false;
};
