#include "AudioPlayModeScript.h"

#include "Object/GameObject.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<AudioPlayModeScript>("AudioPlayModeScript")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr)
        .property("AutoPlay", &AudioPlayModeScript::GetAutoPlay, &AudioPlayModeScript::SetAutoPlay);
}

void AudioPlayModeScript::OnInitialize()
{
    if (auto owner = GetOwner())
    {
        m_Source = owner->GetComponent<AudioSourceComponent>();
    }
}

void AudioPlayModeScript::OnUpdate(float delta)
{
    (void)delta;

    if (!m_AutoPlay)
    {
        return;
    }

    if (!m_Source)
    {
        if (auto owner = GetOwner())
        {
            m_Source = owner->GetComponent<AudioSourceComponent>();
        }
        if (!m_Source)
        {
            return;
        }
    }

    PlayModeState curr = PlayModeSystem::Instance().GetPlayMode();

    if (curr == PlayModeState::Playing)
    {
        if (!m_HasStarted)
        {
            m_Source->Play(true);
            m_HasStarted = true;
            m_WasPaused = false;
        }
        else if (m_WasPaused)
        {
            m_Source->Pause(false);
            m_WasPaused = false;
        }
    }
    else if (curr == PlayModeState::Paused)
    {
        if (!m_WasPaused)
        {
            m_Source->Pause(true);
            m_WasPaused = true;
        }
    }
    else if (curr == PlayModeState::Stopped)
    {
        if (m_HasStarted || m_Source->IsPlaying())
        {
            m_Source->Stop();
        }
        m_HasStarted = false;
        m_WasPaused = false;
    }
}

nlohmann::json AudioPlayModeScript::Serialize()
{
    nlohmann::json data;
    rttr::type t = rttr::type::get(*this);
    data["type"] = t.get_name().to_string();
    data["properties"] = nlohmann::json::object();
    data["properties"]["AutoPlay"] = m_AutoPlay;
    return data;
}

void AudioPlayModeScript::Deserialize(nlohmann::json data)
{
    if (!data.contains("properties"))
    {
        return;
    }
    const auto& props = data["properties"];
    if (props.contains("AutoPlay"))
    {
        m_AutoPlay = props["AutoPlay"].get<bool>();
    }
}

bool AudioPlayModeScript::GetAutoPlay() const
{
    return m_AutoPlay;
}

void AudioPlayModeScript::SetAutoPlay(bool enabled)
{
    m_AutoPlay = enabled;
}
