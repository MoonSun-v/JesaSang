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

//int WorldManager::GetCameraIndex()
//{
//    return cameraIndex;
//}
//
//void WorldManager::SetCameraIndex(int index)
//{
//    cameraIndex = CameraSystem::Instance().SetCurrCamera(index);
//}
