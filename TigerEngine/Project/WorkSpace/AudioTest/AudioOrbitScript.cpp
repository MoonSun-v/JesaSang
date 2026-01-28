#include "AudioOrbitScript.h"

#include <cmath>

#include "Object/GameObject.h"
#include "Components/Transform.h"
#include "Util/ComponentAutoRegister.h"

REGISTER_COMPONENT(AudioOrbitScript);

RTTR_REGISTRATION
{
    rttr::registration::class_<AudioOrbitScript>("AudioOrbitScript")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr)
        .property("Radius", &AudioOrbitScript::radius)
        .property("Speed", &AudioOrbitScript::speed)
        .property("Height", &AudioOrbitScript::height);
}

void AudioOrbitScript::OnInitialize()
{
    auto owner = GetOwner();
    if (!owner)
    {
        return;
    }

    m_Transform = owner->GetTransform();
    m_Source = owner->GetComponent<AudioSourceComponent>();

    if (m_Transform)
    {
        m_Center = m_Transform->GetPosition();
        m_PrevPos = m_Center;
    }
}

void AudioOrbitScript::OnUpdate(float delta)
{
    if (!m_Transform || delta <= 0.0f)
    {
        return;
    }

    m_Time += delta;
    float phase = m_Time * speed;

    constexpr float kSegmentSeconds = 6.0f;
    const float segment = std::fmod(m_Time, kSegmentSeconds * 4.0f);
    const int mode = static_cast<int>(segment / kSegmentSeconds);

    Vector3 pos = m_Center;
    switch (mode)
    {
    case 0: // Left/Right
        pos.x = m_Center.x + std::sin(phase) * radius;
        pos.y = m_Center.y + height;
        break;
    case 1: // Up/Down
        pos.y = m_Center.y + std::sin(phase) * height;
        break;
    case 2: // Forward/Back
        pos.z = m_Center.z + std::sin(phase) * radius;
        pos.y = m_Center.y + height;
        break;
    default: // Diagonal sweep
        pos.x = m_Center.x + std::sin(phase) * radius;
        pos.y = m_Center.y + std::sin(phase) * height;
        pos.z = m_Center.z + std::sin(phase) * radius;
        break;
    }

    m_Transform->SetPosition(pos);

    // Convert cm/s to m/s for FMOD doppler.
    Vector3 vel = (pos - m_PrevPos) / delta * 0.01f;
    m_PrevPos = pos;

    if (m_Source)
    {
        AudioTransform t{};
        t.position = { pos.x, pos.y, pos.z };
        t.velocity = { vel.x, vel.y, vel.z };
        t.forward = { 0.0f, 0.0f, 1.0f };
        t.up = { 0.0f, 1.0f, 0.0f };
        m_Source->SetFallback(t);
    }
}

nlohmann::json AudioOrbitScript::Serialize()
{
    nlohmann::json datas;

    rttr::type t = rttr::type::get(*this);
    datas["type"] = t.get_name().to_string();
    datas["properties"] = nlohmann::json::object();

    datas["properties"]["Radius"] = radius;
    datas["properties"]["Speed"] = speed;
    datas["properties"]["Height"] = height;

    return datas;
}

void AudioOrbitScript::Deserialize(nlohmann::json data)
{
    if (!data.contains("properties"))
    {
        return;
    }

    const auto& props = data["properties"];
    if (props.contains("Radius"))
    {
        radius = props["Radius"].get<float>();
    }
    if (props.contains("Speed"))
    {
        speed = props["Speed"].get<float>();
    }
    if (props.contains("Height"))
    {
        height = props["Height"].get<float>();
    }
}
