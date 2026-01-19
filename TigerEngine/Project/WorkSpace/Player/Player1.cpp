#include "Player1.h"
#include "Object/GameObject.h"
#include "System/InputSystem.h"
#include "Object/Component.h"
#include "Components/FBXRenderer.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<Player1>("Player1")
    .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr)
    .property("r", &Player1::r)
    .property("g", &Player1::g)
    .property("b", &Player1::b);
}

void Player1::OnInitialize()
{
    weapon = GetOwner()->AddComponent<Weapon>();
}

void Player1::OnStart()
{
}

void Player1::OnUpdate(float delta)
{
    auto comp = GetOwner()->GetComponent<FBXRenderer>();
    if (comp != nullptr)
    {
        comp->SetColor({ r, g, b, 1.0f });
    }

    r += delta;
    g += delta * 2.0f;
    b += delta * 3.0f;

    if (r > 1.0f) r = 0.f;
    if (g > 1.0f) g = 0.f;
    if (b > 1.0f) b = 0.f;

    auto trans = GetOwner()->GetTransform();
    if (Input::GetKey(DirectX::Keyboard::Keys::W))
        trans->Translate({ 0, 0, 1.f });
    else if (Input::GetKey(DirectX::Keyboard::Keys::S))
        trans->Translate({ 0, 0, -1.f });
    if (Input::GetKey(DirectX::Keyboard::Keys::A))
        trans->Translate({ -1.f, 0, 0 });
    else if (Input::GetKey(DirectX::Keyboard::Keys::D))
        trans->Translate({ 1.f, 0, 0 });
}

nlohmann::json Player1::Serialize()
{
    nlohmann::json datas;

    rttr::type t = rttr::type::get(*this);
    datas["type"] = t.get_name().to_string();
    datas["properties"] = nlohmann::json::object(); // 객체 생성

    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);
        if (value.is_type<float>())
        {
            auto v = value.get_value<float>();
            datas["properties"][propName] = v;
        }
    }

    return datas;
}

void Player1::Deserialize(nlohmann::json data)
{
    auto propData = data["properties"];

    rttr::type t = rttr::type::get(*this);
    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);
        if (value.is_type<float>() && propName == "r")
        {
            float data = propData["r"];
            prop.set_value(*this, data);
        }
        else if (value.is_type<float>() && propName == "g")
        {
            float data = propData["g"];
            prop.set_value(*this, data);
        }
        else if (value.is_type<Color>() && propName == "b")
        {
            float data = propData["b"];
            prop.set_value(*this, data);
        }
    }
}
