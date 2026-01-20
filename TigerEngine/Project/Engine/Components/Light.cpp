#include "Light.h"
#include "../EngineSystem/LightSystem.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<Light>("Light")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr)

        .property("type", &Light::type)
        .property("isSunLight", &Light::isSunLight)

        .property("color", &Light::color)
        .property("intensity", &Light::intensity)

        .property("direction", &Light::direction)
        .property("position", &Light::position)

        .property("range", &Light::range)
        .property("innerAngle", &Light::innerAngle)
        .property("outerAngle", &Light::outerAngle);
}


Light::Light()
{
    // member init
    type = LightType::Directional;
    isSunLight = false;
    color = { 1.0f, 1.0f, 1.0f };
    intensity = 1.0f;
    direction = { 0.0f, -1.0f, 0.0f };
    position = { 0.0f, 0.0f, 0.0f };
    range = 15.0f;
    innerAngle = 15.0f;
    outerAngle = 30.0f;
}

void Light::OnInitialize()
{
    LightSystem::Instance().Register(this);
}

void Light::OnDestory()
{
    LightSystem::Instance().UnRegister(this);
}

nlohmann::json Light::Serialize()
{
    nlohmann::json datas;

    rttr::type t = rttr::type::get(*this);
    datas["type"] = t.get_name().to_string();
    datas["properties"] = nlohmann::json::object();

    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);

        // LightType
        if (propName == "type")
        {
            datas["properties"][propName] = static_cast<int>(type);
        }
        // bool
        else if (value.is_type<bool>())
        {
            datas["properties"][propName] = value.get_value<bool>();
        }
        // float
        else if (value.is_type<float>())
        {
            datas["properties"][propName] = value.get_value<float>();
        }
        // Vector3
        else if (value.is_type<Vector3>())
        {
            datas["properties"][propName] = Vec3ToJson(value.get_value<Vector3>());
        }
    }

    return datas;
}

void Light::Deserialize(nlohmann::json data)
{
    if (!data.is_object() || !data.contains("properties"))
        return;

    auto propData = data["properties"];
    rttr::type t = rttr::type::get(*this);

    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        if (!propData.contains(propName))
            continue;

        // LightType  // TODO :: enum class 어떻게 들어오는지 확인 필요
        if (propName == "type")
        {
            auto v = propData[propName].get<int>();
            prop.set_value(*this, v);
        }
        // bool
        else if (propName == "isSunLight")
        {
            auto v = propData[propName].get<bool>();
            prop.set_value(*this, v);
        }
        // float
        else if (propName == "intensity")
        {
            auto v = propData[propName].get<float>();
            prop.set_value(*this, v);
        }
        else if (propName == "range")
        {
            auto v = propData[propName].get<float>();
            prop.set_value(*this, v);
        }
        else if (propName == "innerAngle")
        {
            auto v = propData[propName].get<float>();
            prop.set_value(*this, v);
        }
        else if (propName == "outerAngle")
        {
            auto v = propData[propName].get<float>();
            prop.set_value(*this, v);
        }
        // Vector3 // TODO :: 유틸 적용 되는지 확인 필요
        else if (propName == "color")
        {
            color = JsonToVec3(propData[propName], color);
        }
        else if (propName == "direction")
        {
            direction = JsonToVec3(propData[propName], direction);
        }
        else if (propName == "position")
        {
            position = JsonToVec3(propData[propName], position);
        }
    }
}

// Utils ----------------
nlohmann::json Vec3ToJson(const Vector3& v)
{
    return nlohmann::json{
        {"x", v.x},
        {"y", v.y},
        {"z", v.z}
    };
}

Vector3 JsonToVec3(const nlohmann::json& j, const Vector3& fallback)
{
    if (!j.is_object()) return fallback;

    Vector3 v = fallback;
    if (j.contains("x")) v.x = j["x"].get<float>();
    if (j.contains("y")) v.y = j["y"].get<float>();
    if (j.contains("z")) v.z = j["z"].get<float>();
    return v;
}