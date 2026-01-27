#include "Light.h"
#include "../EngineSystem/LightSystem.h"
#include "../Util/JsonHelper.h"

RTTR_REGISTRATION
{
    rttr::registration::enumeration<LightType>("LightType")
        (
            rttr::value("Directional", LightType::Directional),
            rttr::value("Point", LightType::Point),
            rttr::value("Spot", LightType::Spot)
        );

    rttr::registration::class_<Light>("Light")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr)

        .property("LightType",  &Light::type)
        .property("IsSunLight", &Light::isSunLight)

        .property("Color",      &Light::GetColor, &Light::SetColor)
        .property("Intensity",  &Light::intensity)

        .property("Direction",  &Light::direction)
        .property("Range",      &Light::range)
        .property("InnerAngle", &Light::innerAngle)
        .property("OuterAngle", &Light::outerAngle);
}


Light::Light()
{
    // member init
    type = LightType::Directional;
    isSunLight = false;
    color = { 1.0f, 1.0f, 1.0f };
    intensity = 1.0f;
    direction = { 0.0f, -0.3f, 1.0f };
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
    return JsonHelper::MakeSaveData(this);
}

void Light::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}