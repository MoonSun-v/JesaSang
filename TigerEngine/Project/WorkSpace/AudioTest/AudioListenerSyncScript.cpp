#include "AudioListenerSyncScript.h"

#include "Object/GameObject.h"
#include "Components/Transform.h"
#include "Util/ComponentAutoRegister.h"

REGISTER_COMPONENT(AudioListenerSyncScript);

RTTR_REGISTRATION
{
    rttr::registration::class_<AudioListenerSyncScript>("AudioListenerSyncScript")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr);
}

void AudioListenerSyncScript::OnInitialize()
{
    auto owner = GetOwner();
    if (!owner)
    {
        return;
    }

    m_Transform = owner->GetTransform();
    m_Listener = owner->GetComponent<AudioListenerComponent>();

    if (m_Transform)
    {
        m_PrevPos = m_Transform->GetPosition();
    }
}

void AudioListenerSyncScript::OnUpdate(float delta)
{
    if (!m_Transform || !m_Listener || delta <= 0.0f)
    {
        return;
    }

    const Vector3 pos = m_Transform->GetPosition();
    // Convert cm/s to m/s for FMOD doppler.
    Vector3 vel = (pos - m_PrevPos) / delta * 0.01f;
    m_PrevPos = pos;

    AudioTransform t{};
    t.position = { pos.x, pos.y, pos.z };
    t.velocity = { vel.x, vel.y, vel.z };
    t.forward = { 0.0f, 0.0f, 1.0f };
    t.up = { 0.0f, 1.0f, 0.0f };
    m_Listener->SetFallback(t);
}

nlohmann::json AudioListenerSyncScript::Serialize()
{
    nlohmann::json datas;
    rttr::type t = rttr::type::get(*this);
    datas["type"] = t.get_name().to_string();
    datas["properties"] = nlohmann::json::object();
    return datas;
}

void AudioListenerSyncScript::Deserialize(nlohmann::json data)
{
    (void)data;
}
