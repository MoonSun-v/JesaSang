#include "..\\include\\AudioListenerComponent.h"
#include "..\\include\\AudioSystem.h"

void AudioListenerComponent::Init(AudioSystem* system)
{
    m_System = system;
}

void AudioListenerComponent::BindTransform(const AudioTransformRef& ref)
{
    m_Bind = ref;
}

void AudioListenerComponent::SetFallback(const AudioTransform& transform)
{
    m_Fallback = transform;
}

void AudioListenerComponent::Update()
{
    if (!m_System || !m_System->GetSystem()) {
        return;
    }

    const AudioTransform t = ResolveAudioTransform(m_Fallback, m_Bind);
    m_Listener.Set(t.position, t.velocity, t.forward, t.up);
    m_Listener.Apply(m_System->GetSystem());
}
