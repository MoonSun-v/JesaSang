#include "AudioSourceComponent.h"

#include "..\\Manager\\AudioManager.h"
#include "..\\..\\Externals\\AudioModule_FMOD\\include\\AudioClip.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<AudioSourceComponent>("AudioSourceComponent")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr)
        .property("ClipId", &AudioSourceComponent::GetClipId, &AudioSourceComponent::SetClipId)
        .property("Volume", &AudioSourceComponent::GetVolume, &AudioSourceComponent::SetVolume)
        .property("Loop", &AudioSourceComponent::GetLoop, &AudioSourceComponent::SetLoop)
        .property("Pitch", &AudioSourceComponent::GetPitch, &AudioSourceComponent::SetPitch)
        .property("MinDistance", &AudioSourceComponent::GetMinDistance, &AudioSourceComponent::SetMinDistance)
        .property("MaxDistance", &AudioSourceComponent::GetMaxDistance, &AudioSourceComponent::SetMaxDistance);
}

void AudioSourceComponent::OnInitialize()
{
    Init(&AudioManager::Instance().GetSystem());
}

void AudioSourceComponent::OnUpdate(float delta)
{
    (void)delta;
    Update3D();
}

void AudioSourceComponent::OnDestory()
{
    Stop();
    m_System = nullptr;
}

nlohmann::json AudioSourceComponent::Serialize()
{
    nlohmann::json datas;

    rttr::type t = rttr::type::get(*this);
    datas["type"] = t.get_name().to_string();
    datas["properties"] = nlohmann::json::object();

    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);
        if (value.is_type<std::string>() && propName == "ClipId")
        {
            datas["properties"][propName] = value.get_value<std::string>();
        }
        else if (value.is_type<float>())
        {
            datas["properties"][propName] = value.get_value<float>();
        }
        else if (value.is_type<bool>() && propName == "Loop")
        {
            datas["properties"][propName] = value.get_value<bool>();
        }
    }

    return datas;
}

void AudioSourceComponent::Deserialize(nlohmann::json data)
{
    if (!data.contains("properties"))
    {
        return;
    }

    const auto& propData = data["properties"];

    if (propData.contains("ClipId"))
    {
        m_ClipId = propData["ClipId"].get<std::string>();
    }
    if (propData.contains("Volume"))
    {
        m_Volume = propData["Volume"].get<float>();
    }
    if (propData.contains("Loop"))
    {
        m_Loop = propData["Loop"].get<bool>();
    }
    if (propData.contains("Pitch"))
    {
        m_Pitch = propData["Pitch"].get<float>();
    }
    if (propData.contains("MinDistance"))
    {
        m_MinDistance = propData["MinDistance"].get<float>();
    }
    if (propData.contains("MaxDistance"))
    {
        m_MaxDistance = propData["MaxDistance"].get<float>();
    }

    if (!m_ClipId.empty())
    {
        auto clip = AudioManager::Instance().GetOrCreateClip(m_ClipId);
        if (clip)
        {
            m_Source.SetClip(std::move(clip));
        }
    }

    m_Source.SetVolume(m_Volume);
    m_Source.SetLoop(m_Loop);
    m_Source.SetPitch(m_Pitch);
    m_Source.Set3DMinMaxDistance(m_MinDistance, m_MaxDistance);
}

void AudioSourceComponent::Init(AudioSystem* system)
{
    m_System = system;
    if (m_System && m_System->GetSystem())
    {
        m_Source.Init(m_System->GetSystem());
    }
}

void AudioSourceComponent::BindTransform(const AudioTransformRef& ref)
{
    m_Bind = ref;
}

void AudioSourceComponent::SetFallback(const AudioTransform& transform)
{
    m_Fallback = transform;
}

const std::string& AudioSourceComponent::GetClipId() const
{
    return m_ClipId;
}

void AudioSourceComponent::SetClipId(const std::string& id)
{
    m_ClipId = id;
    if (m_ClipId.empty())
    {
        return;
    }

    auto clip = AudioManager::Instance().GetOrCreateClip(m_ClipId);
    if (clip)
    {
        m_Source.SetClip(std::move(clip));
    }
}

bool AudioSourceComponent::GetLoop() const
{
    return m_Loop;
}

void AudioSourceComponent::SetClip(std::shared_ptr<AudioClip> clip)
{
    m_Source.SetClip(std::move(clip));
}

void AudioSourceComponent::SetLoop(bool loop)
{
    m_Loop = loop;
    m_Source.SetLoop(loop);
}

float AudioSourceComponent::GetVolume() const
{
    return m_Volume;
}

void AudioSourceComponent::SetVolume(float volume)
{
    m_Volume = volume;
    m_Source.SetVolume(volume);
}

float AudioSourceComponent::GetPitch() const
{
    return m_Pitch;
}

void AudioSourceComponent::SetPitch(float pitch)
{
    m_Pitch = pitch;
    m_Source.SetPitch(pitch);
}

float AudioSourceComponent::GetMinDistance() const
{
    return m_MinDistance;
}

float AudioSourceComponent::GetMaxDistance() const
{
    return m_MaxDistance;
}

void AudioSourceComponent::Set3DMinMaxDistance(float minDist, float maxDist)
{
    m_MinDistance = minDist;
    m_MaxDistance = maxDist;
    m_Source.Set3DMinMaxDistance(minDist, maxDist);
}

void AudioSourceComponent::SetMinDistance(float minDist)
{
    m_MinDistance = minDist;
    m_Source.Set3DMinMaxDistance(m_MinDistance, m_MaxDistance);
}

void AudioSourceComponent::SetMaxDistance(float maxDist)
{
    m_MaxDistance = maxDist;
    m_Source.Set3DMinMaxDistance(m_MinDistance, m_MaxDistance);
}

void AudioSourceComponent::Play(bool restart)
{
    m_Source.Play(restart);
}

void AudioSourceComponent::PlayOneShot()
{
    m_Source.PlayOneShot();
}

void AudioSourceComponent::Stop()
{
    m_Source.Stop();
}

void AudioSourceComponent::Update3D()
{
    const AudioTransform t = ResolveAudioTransform(m_Fallback, m_Bind);
    m_Source.SetPosition(t.position);
    m_Source.SetVelocity(t.velocity);
    m_Source.Update3D();
}

bool AudioSourceComponent::IsPlaying() const
{
    return m_Source.IsPlaying();
}
