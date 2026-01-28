#include "AudioKeyTriggerScript.h"

#include "System/InputSystem.h"
#include "Object/GameObject.h"
#include "Util/ComponentAutoRegister.h"
#include "Manager/AudioManager.h"

REGISTER_COMPONENT(AudioKeyTriggerScript);

RTTR_REGISTRATION
{
    rttr::registration::class_<AudioKeyTriggerScript>("AudioKeyTriggerScript")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr)
        .property("ClipId", &AudioKeyTriggerScript::clipId);
}

void AudioKeyTriggerScript::OnInitialize()
{
    if (auto owner = GetOwner())
    {
        m_Source = owner->GetComponent<AudioSourceComponent>();
    }
}

void AudioKeyTriggerScript::OnUpdate(float delta)
{
    (void)delta;

    if (Input::GetKeyDown(DirectX::Keyboard::Keys::Z))
    {
        if (!m_Source)
        {
            if (auto owner = GetOwner())
            {
                m_Source = owner->GetComponent<AudioSourceComponent>();
            }
        }
        if (!m_Source)
        {
            return;
        }

        auto clip = AudioManager::Instance().GetOrCreateClip(clipId);
        if (clip)
        {
            m_Source->SetClip(std::move(clip));
            m_Source->SetLoop(false);
            m_Source->PlayOneShot();
        }
    }
}

nlohmann::json AudioKeyTriggerScript::Serialize()
{
    nlohmann::json data;
    rttr::type t = rttr::type::get(*this);
    data["type"] = t.get_name().to_string();
    data["properties"] = nlohmann::json::object();
    data["properties"]["ClipId"] = clipId;
    return data;
}

void AudioKeyTriggerScript::Deserialize(nlohmann::json data)
{
    if (!data.contains("properties"))
    {
        return;
    }

    const auto& props = data["properties"];
    if (props.contains("ClipId"))
    {
        clipId = props["ClipId"].get<std::string>();
    }
}
