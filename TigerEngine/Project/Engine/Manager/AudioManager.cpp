#include "AudioManager.h"

#include "../Util/PathHelper.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace
{
    std::string ToUpper(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return value;
    }

    bool IsTruthy(const std::string& value)
    {
        if (value.empty())
        {
            return false;
        }

        std::string upper = value;
        upper = ToUpper(upper);
        return upper == "1" || upper == "TRUE" || upper == "YES";
    }

    std::filesystem::path ResolveAudioPath(const std::string& rawPath)
    {
        std::filesystem::path input(rawPath);
        if (input.is_absolute())
        {
            return input;
        }

        static std::optional<std::filesystem::path> audioDir =
            PathHelper::FindDirectory("Assets\\Audio");
        if (!audioDir)
        {
            return input;
        }

        std::filesystem::path tail;
        bool seenAssets = false;
        bool seenAudio = false;
        for (const auto& part : input)
        {
            std::string partUpper = ToUpper(part.string());
            if (!seenAssets)
            {
                if (partUpper == "ASSETS")
                {
                    seenAssets = true;
                }
                continue;
            }
            if (seenAssets && !seenAudio)
            {
                if (partUpper == "AUDIO")
                {
                    seenAudio = true;
                }
                else
                {
                    seenAssets = false;
                }
                continue;
            }
            if (seenAudio)
            {
                tail /= part;
            }
        }

        if (seenAudio)
        {
            if (tail.empty())
            {
                return *audioDir;
            }
            return (*audioDir / tail).lexically_normal();
        }

        return (*audioDir / input).lexically_normal();
    }
}

bool AudioManager::Initialize(const std::string& manifestPath)
{
    if (m_initialized)
    {
        return true;
    }

    m_manifestPath = ResolveManifestPath(manifestPath);
    if (m_manifestPath.empty())
    {
        OutputDebugStringA("[AudioManager] Failed to locate AudioManifest.csv\n");
        return false;
    }

    if (!m_system.Init())
    {
        OutputDebugStringA("[AudioManager] AudioSystem init failed\n");
        return false;
    }

    if (!LoadManifest(m_manifestPath.string()))
    {
        OutputDebugStringA("[AudioManager] LoadManifest failed\n");
        return false;
    }

    m_initialized = true;
    return true;
}

void AudioManager::Shutdown()
{
    m_clips.clear();
    m_entries.clear();
    m_system.Shutdown();
    m_initialized = false;
}

void AudioManager::Update()
{
    m_system.Update();
}

bool AudioManager::LoadManifest(const std::string& manifestPath)
{
    auto resolved = ResolveManifestPath(manifestPath);
    if (resolved.empty())
    {
        return false;
    }

    std::ifstream file(resolved);
    if (!file.is_open())
    {
        return false;
    }

    m_entries.clear();

    std::string line;
    while (std::getline(file, line))
    {
        line = Trim(line);
        if (line.empty())
        {
            continue;
        }
        if (line.rfind("#", 0) == 0 || line.rfind("//", 0) == 0)
        {
            continue;
        }

        auto cols = SplitCsvLine(line);
        if (cols.empty())
        {
            continue;
        }

        if (cols.size() >= 1)
        {
            std::string header = ToUpper(cols[0]);
            if (header == "ID")
            {
                continue;
            }
        }

        if (cols.size() < 6)
        {
            continue;
        }

        AudioManifestEntry entry{};
        entry.id = Trim(cols[0]);
        entry.path = Trim(cols[1]);
        entry.group = Trim(cols[2]);
        entry.mode = ParseMode(cols[3]);
        entry.defaultVolume = std::stof(Trim(cols[4]));
        entry.loop = IsTruthy(Trim(cols[5]));

        if (!entry.id.empty())
        {
            m_entries[entry.id] = entry;
        }
    }

    m_manifestPath = resolved;
    return true;
}

const AudioManifestEntry* AudioManager::GetEntry(const std::string& id) const
{
    auto it = m_entries.find(id);
    if (it == m_entries.end())
    {
        return nullptr;
    }
    return &it->second;
}

std::shared_ptr<AudioClip> AudioManager::GetOrCreateClip(const std::string& id)
{
    auto it = m_clips.find(id);
    if (it != m_clips.end())
    {
        return it->second;
    }

    auto entry = GetEntry(id);
    if (!entry)
    {
        return {};
    }

    auto clip = m_system.CreateClip(ResolveAudioPath(entry->path).string(), entry->mode);
    if (clip)
    {
        m_clips.emplace(id, clip);
    }
    return clip;
}

void AudioManager::PreloadAll()
{
    for (const auto& pair : m_entries)
    {
        if (m_clips.find(pair.first) != m_clips.end())
        {
            continue;
        }
        auto clip = m_system.CreateClip(ResolveAudioPath(pair.second.path).string(), pair.second.mode);
        if (clip)
        {
            m_clips.emplace(pair.first, clip);
        }
    }
}

std::filesystem::path AudioManager::ResolveManifestPath(const std::string& manifestPath) const
{
    if (!manifestPath.empty())
    {
        return manifestPath;
    }

    auto audioDir = PathHelper::FindDirectory("Assets\\Audio");
    if (!audioDir)
    {
        return {};
    }

    return *audioDir / "AudioManifest.csv";
}

std::string AudioManager::Trim(std::string value)
{
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());

    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
    {
        value = value.substr(1, value.size() - 2);
    }

    return value;
}

std::vector<std::string> AudioManager::SplitCsvLine(const std::string& line)
{
    std::vector<std::string> cols;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ','))
    {
        cols.push_back(item);
    }
    return cols;
}

FMOD_MODE AudioManager::ParseMode(const std::string& text)
{
    std::string upper = ToUpper(text);

    if (upper == "3D" || upper == "3D_INVERSE" || upper == "INVERSE")
    {
        return FMOD_3D | FMOD_3D_INVERSEROLLOFF;
    }
    if (upper == "3D_LINEAR" || upper == "LINEAR")
    {
        return FMOD_3D | FMOD_3D_LINEARROLLOFF;
    }
    if (upper == "3D_LINEAR_SQUARE" || upper == "LINEAR_SQUARE")
    {
        return FMOD_3D | FMOD_3D_LINEARSQUAREROLLOFF;
    }
    if (upper == "3D_CUSTOM" || upper == "CUSTOM")
    {
        return FMOD_3D | FMOD_3D_CUSTOMROLLOFF;
    }
    if (upper == "2D")
    {
        return FMOD_2D;
    }
    return FMOD_DEFAULT;
}
