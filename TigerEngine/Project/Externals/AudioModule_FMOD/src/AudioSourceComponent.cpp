#include "AudioSourceComponent.h"
#include "AudioSystem.h"

void AudioSourceComponent::Init(AudioSystem* system)
{
    m_System = system;
    if (m_System && m_System->GetSystem()) {
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

void AudioSourceComponent::SetClip(std::shared_ptr<AudioClip> clip)
{
    m_Source.SetClip(std::move(clip));
}

void AudioSourceComponent::SetLoop(bool loop)
{
    m_Source.SetLoop(loop);
}

void AudioSourceComponent::SetVolume(float volume)
{
    m_Source.SetVolume(volume);
}

void AudioSourceComponent::SetPitch(float pitch)
{
    m_Source.SetPitch(pitch);
}

void AudioSourceComponent::Set3DMinMaxDistance(float minDist, float maxDist)
{
    m_Source.Set3DMinMaxDistance(minDist, maxDist);
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
