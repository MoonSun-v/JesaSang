#include "AudioListenerComponent.h"

#include "..\\Manager\\AudioManager.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<AudioListenerComponent>("AudioListenerComponent")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr);
}

void AudioListenerComponent::OnInitialize()
{
    Init(&AudioManager::Instance().GetSystem());
}

void AudioListenerComponent::OnUpdate(float delta)
{
    (void)delta;
    Update();
}

void AudioListenerComponent::OnDestory()
{
    m_System = nullptr;
}

nlohmann::json AudioListenerComponent::Serialize()
{
    nlohmann::json datas;

    rttr::type t = rttr::type::get(*this);
    datas["type"] = t.get_name().to_string();
    datas["properties"] = nlohmann::json::object();

    return datas;
}

void AudioListenerComponent::Deserialize(nlohmann::json data)
{
    (void)data;
}

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
    if (!m_System || !m_System->GetSystem())
    {
        return;
    }

    const AudioTransform t = ResolveAudioTransform(m_Fallback, m_Bind);
    m_Listener.Set(t.position, t.velocity, t.forward, t.up);
    m_Listener.Apply(m_System->GetSystem());
}
