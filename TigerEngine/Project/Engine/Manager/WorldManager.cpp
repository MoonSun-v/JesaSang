#include "WorldManager.h"
#include "ShaderManager.h"
#include "../Components/Camera.h"
#include "../Object/GameObject.h"
#include "../../Base/System/TimeSystem.h"

#include "Datas/ReflectionMedtaDatas.hpp"

RTTR_REGISTRATION
{
    using namespace rttr;

    registration::class_<WorldManager>("WorldManager")
        .property("useIBL",            &WorldManager::useIBL)
            (metadata(META_BOOL, true))
        .property("indirectIntensity", &WorldManager::indirectIntensity)
        .property("shadowData",        &WorldManager::shadowData)
        .property("postProcessData",   &WorldManager::postProcessData)
        .property("frameData",         &WorldManager::frameData);

    registration::class_<ShadowOrthoDesc>("ShadowOrthoDesc")
        .constructor<>()
        .property("lookPointDist",    &ShadowOrthoDesc::lookPointDist)
        .property("shadowLightDist",  &ShadowOrthoDesc::shadowLightDist)
        .property("shadowWidth",      &ShadowOrthoDesc::shadowWidth)
        .property("shadowHeight",     &ShadowOrthoDesc::shadowHeight)
        .property("shadowNear",       &ShadowOrthoDesc::shadowNear)
        .property("shadowFar",        &ShadowOrthoDesc::shadowFar);

    registration::class_<FrameWorldData>("FrameWorldData")
        .constructor<>()
        .property("time",          &FrameWorldData::time)
        .property("deltaTime",     &FrameWorldData::deltaTime)
        .property("screenSize",    &FrameWorldData::screenSize)
        .property("shadowMapSize", &FrameWorldData::shadowMapSize)
        .property("cameraPos",     &FrameWorldData::cameraPos);

    registration::class_<PostProcessWorldData>("PostProcessWorldData")
        .constructor<>()

        // Base
        .property("isHDR",           &PostProcessWorldData::isHDR)
            (metadata(META_BOOL, true))
        .property("useDefaultGamma", &PostProcessWorldData::useDefaultGamma)
            (metadata(META_BOOL, true))
        .property("defaultGamma",    &PostProcessWorldData::defaultGamma)
        .property("exposure",        &PostProcessWorldData::exposure)

        // Enable
        .property("useColorAdjustments", &PostProcessWorldData::useColorAdjustments)
            (metadata(META_BOOL, true))
        .property("useWhiteBalance", &PostProcessWorldData::useWhiteBalance)
            (metadata(META_BOOL, true))
        .property("useLGG",          &PostProcessWorldData::useLGG)
            (metadata(META_BOOL, true))
        .property("useVignette",     &PostProcessWorldData::useVignette)
            (metadata(META_BOOL, true))
        .property("useFilmGrain",    &PostProcessWorldData::useFilmGrain)
            (metadata(META_BOOL, true))
        .property("useBloom",        &PostProcessWorldData::useBloom)
            (metadata(META_BOOL, true))

        // Color Adjustments
        .property("contrast",        &PostProcessWorldData::contrast)
        .property("saturation",      &PostProcessWorldData::saturation)
        .property("useHueShift",     &PostProcessWorldData::useHueShift)
            (metadata(META_BOOL, true))
        .property("hueShift",        &PostProcessWorldData::hueShift)
        .property("useColorTint",    &PostProcessWorldData::useColorTint)
            (metadata(META_BOOL, true))
        .property("colorTint",       &PostProcessWorldData::colorTint)
        .property("colorTint_strength", &PostProcessWorldData::colorTint_strength)

        // White Balance
        .property("temperature",     &PostProcessWorldData::temperature)
        .property("tint",            &PostProcessWorldData::tint)

        // Lift / Gamma / Gain
        .property("useLift",         &PostProcessWorldData::useLift)
            (metadata(META_BOOL, true))
        .property("useGamma",        &PostProcessWorldData::useGamma)
            (metadata(META_BOOL, true))
        .property("useGain",         &PostProcessWorldData::useGain)
            (metadata(META_BOOL, true))

        .property("lift",            &PostProcessWorldData::lift)
        .property("lift_strength",   &PostProcessWorldData::lift_strength)
        .property("gamma",           &PostProcessWorldData::gamma)
        .property("gamma_strength",  &PostProcessWorldData::gamma_strength)
        .property("gain",            &PostProcessWorldData::gain)
        .property("gain_strength",   &PostProcessWorldData::gain_strength)

        // Vignette
        .property("vignette_intensity",  &PostProcessWorldData::vignette_intensity)
        .property("vignette_smoothness", &PostProcessWorldData::vignette_smoothness)
        .property("vignetteCenter",      &PostProcessWorldData::vignetteCenter)
        .property("vignetteColor",       &PostProcessWorldData::vignetteColor)

        // FilmGrain
        .property("grain_intensity", &PostProcessWorldData::grain_intensity)
        .property("grain_response",  &PostProcessWorldData::grain_response)
        .property("grain_scale",     &PostProcessWorldData::grain_scale)

        // Bloom
        .property("bloom_threshold", &PostProcessWorldData::bloom_threshold)
        .property("bloom_intensity", &PostProcessWorldData::bloom_intensity)
        .property("bloom_scatter",   &PostProcessWorldData::bloom_scatter)
        .property("bloom_clamp",     &PostProcessWorldData::bloom_clamp)
        .property("bloom_tint",      &PostProcessWorldData::bloom_tint);
}

void WorldManager::Update(const ComPtr<ID3D11DeviceContext>& context, Camera* camera,
    int clientWidth, int clientHeight)
{
    auto& sm = ShaderManager::Instance();

    // Frame CB Update
    sm.frameCBData.time = GameTimer::Instance().TotalTime();
    sm.frameCBData.deltaTime = GameTimer::Instance().DeltaTime();
    sm.frameCBData.screenSize = { (float)clientWidth,(float)clientHeight };
    sm.frameCBData.cameraPos = camera->GetOwner()->GetTransform()->GetPosition();
    context->UpdateSubresource(sm.frameCB.Get(), 0, nullptr, &sm.frameCBData, 0, 0);
}

nlohmann::json WorldManager::Serialize()
{
    nlohmann::json datas;
    rttr::type t = rttr::type::get(*this);
    datas["type"] = t.get_name().to_string();
    datas["properties"] = nlohmann::json::object(); 

    // World에 있는 구조체 말고 일반 자료형들 저장
    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);

        if (propName == "ShadowOrthoDesc" ||
            propName == "PostProcessWorldData" ||
            propName == "FrameWorldData") continue;

        if (value.is_type<int>())
        {
            datas["properties"][propName] = value.get_value<int>();
        }
        else if (value.is_type<float>())
        {
            datas["properties"][propName] = value.get_value<float>();
        }
        else if (value.is_type<BOOL>() && prop.get_metadata(META_BOOL))
        {
            datas["properties"][propName] = value.get_value<int>(); // BOOL는 0 : false, 1: true?
        }
    }

    // 나머지 클래스, 구조체 내용 저장
    datas["properties"]["ShadowOrthoDesc"] = SerializeShadowData();
    datas["properties"]["PostProcessWorldData"] = SerializePostProcessData();

    return datas;
}

void WorldManager::Deserialize(nlohmann::json data)
{
    // World에 있는 일반 자료형들
    auto propData = data["properties"];
    rttr::type t = rttr::type::get(*this);
    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);

        auto v = propData[propName];
        if (value.is_type<int>())
        {
            // NOTE : BOOL은 어차피 int 이기 때문에 int로 변환
            prop.set_value(*this, propData[propName].get<int>());
        }
        else if (value.is_type<float>())
        {
            prop.set_value(*this, propData[propName].get<float>());
        }
    }

    // 그 외 나머지 구조체
    DeserializeShadowData(propData["ShadowOrthoDesc"]);
    DeserializePostProcessData(propData["PostProcessWorldData"]);
}

nlohmann::json WorldManager::SerializeShadowData()
{
    nlohmann::json datas;
    rttr::type t = rttr::type::get(this->shadowData);
    datas["type"] = t.get_name().to_string();
    datas["properties"] = nlohmann::json::object();

    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(this->shadowData);

        if (value.is_type<int>())
        {
            datas["properties"][propName] = value.get_value<int>();
        }
        else if (value.is_type<float>())
        {
            datas["properties"][propName] = value.get_value<float>();
        }
        else if (value.is_type<Vector2>())
        {
            auto v = value.get_value<Vector2>();
            datas["properties"][propName] = { v.x, v.y };
        }
        else if (value.is_type<Vector3>())
        {
            auto v = value.get_value<Vector3>();
            datas["properties"][propName] = { v.x, v.y, v.z };
        }
    }

    return datas;
}

void WorldManager::DeserializeShadowData(nlohmann::json data)
{
    // data : data["objects"]["properties"]["구조체 이름"]["구조체 요소 이름1..2.."]
    auto propData = data["properties"];

    rttr::type t = rttr::type::get(this->shadowData);
    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(this->shadowData);
        if (value.is_type<Vector2>())
        {
            Vector2 value = { propData[propName][0], propData[propName][1] };
            prop.set_value((this->shadowData), value);
        }
        else if (value.is_type<Vector3>())
        {
            Vector3 value = { propData[propName][0], propData[propName][1], propData[propName][2] };
            prop.set_value((this->shadowData), value);
        }
        else if (value.is_type<int>())
        {
            prop.set_value((this->shadowData), propData[propName].get<int>());
        }
        else if (value.is_type<float>())
        {
            prop.set_value((this->shadowData), propData[propName].get<float>());
        }
    }
}

nlohmann::json WorldManager::SerializePostProcessData()
{
    nlohmann::json datas;
    rttr::type t = rttr::type::get(this->postProcessData);
    datas["type"] = t.get_name().to_string();
    datas["properties"] = nlohmann::json::object();

    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(this->postProcessData);

        if (value.is_type<int>())
        {
            datas["properties"][propName] = value.get_value<int>();
        }
        else if (value.is_type<float>())
        {
            datas["properties"][propName] = value.get_value<float>();
        }
        else if (value.is_type<Vector2>())
        {
            auto v = value.get_value<Vector2>();
            datas["properties"][propName] = { v.x, v.y };
        }
        else if (value.is_type<Vector3>())
        {
            auto v = value.get_value<Vector3>();
            datas["properties"][propName] = { v.x, v.y, v.z };
        }
    }

    return datas;
}

void WorldManager::DeserializePostProcessData(nlohmann::json data)
{
    auto propData = data["properties"];

    rttr::type t = rttr::type::get(this->postProcessData);
    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(this->postProcessData);
        if (value.is_type<Vector2>())
        {
            Vector2 value = { propData[propName][0], propData[propName][1] };
            prop.set_value((this->postProcessData), value);
        }
        else if (value.is_type<Vector3>())
        {
            Vector3 value = { propData[propName][0], propData[propName][1], propData[propName][2] };
            prop.set_value((this->postProcessData), value);
        }
        else if (value.is_type<int>())
        {
            prop.set_value((this->postProcessData), propData[propName].get<int>());
        }
        else if (value.is_type<float>())
        {
            prop.set_value((this->postProcessData), propData[propName].get<float>());
        }
    }
}
