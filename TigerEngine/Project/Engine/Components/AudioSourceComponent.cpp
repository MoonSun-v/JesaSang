#include "AudioSourceComponent.h"

#include "..\\Manager\\AudioManager.h"
#include "..\\Util\\JsonHelper.h"
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
        .property("ChannelGroup", &AudioSourceComponent::GetChannelGroup, &AudioSourceComponent::SetChannelGroup)
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
    return JsonHelper::MakeSaveData(this);
}

void AudioSourceComponent::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
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

    if (m_ChannelGroup.empty())
    {
        if (const auto* entry = AudioManager::Instance().GetEntry(m_ClipId))
        {
            if (!entry->group.empty())
            {
                SetChannelGroup(entry->group);
            }
        }
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
    if (volume < 0.0f)
    {
        volume = 0.0f;
    }
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

const std::string& AudioSourceComponent::GetChannelGroup() const
{
    return m_ChannelGroup;
}

void AudioSourceComponent::SetChannelGroup(const std::string& name)
{
    m_ChannelGroup = name;
    auto& system = AudioManager::Instance().GetSystem();
    FMOD::ChannelGroup* group = system.GetOrCreateChannelGroup(m_ChannelGroup);
    m_Source.SetChannelGroup(group);
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

void AudioSourceComponent::Pause(bool paused)
{
    m_Source.Pause(paused);
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
