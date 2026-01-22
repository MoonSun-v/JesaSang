#pragma once
#include <pch.h>

struct ShadowOrthoDesc
{
    float lookPointDist = 500.f;
    float shadowLightDist = 1000.f;
    float shadowWidth = 3000.0f;
    float shadowHeight = 3000.0f;
    float shadowNear = 0.01f;
    float shadowFar = 5000.0f;
};

struct PostProcessWorldData
{
    // Base
    BOOL    isHDR = 1;               // LDR/HDR
    BOOL    useDefaultGamma = 1;     // Linear -> SRGB
    float   defaultGamma = 2.2f;     // Gamma (defalut)
    float   exposure = 0;            // 노출

    // Enable
    BOOL useColorAdjustments = 0;
    BOOL useWhiteBalance = 0;
    BOOL useLGG = 0;
    BOOL useVignette = 0;
    BOOL useFilmGrain = 0;
    BOOL useBloom = 0;

    // Color Adjustments (대비, 채도, Hue Shift, Tint)
    float   contrast = 1;
    float   saturation = 1;
    BOOL    useHueShift = false;
    float   hueShift = 0;
    BOOL    useColorTint = false;
    Vector3 colorTint = { 1,1,1 };
    float   colorTint_strength = 0.5;

    // White Balance (온도, 색조)
    float temperature = 0;
    float tint = 0;

    // Lift / Gamma / Gain (어두운톤, 미드톤, 밝은톤 밝기 조정)
    BOOL  useLift = false;
    BOOL  useGamma = false;
    BOOL  useGain = false;

    Vector3 lift = { 0,0,0 };
    float   lift_strength = 0.5;
    Vector3 gamma = { 0,0,0 };
    float   gamma_strength = 1.0;
    Vector3 gain = { 0,0,0 };
    float   gain_strength = 0.5;

    // Vinette
    float   vignette_intensity = 0.5;
    float   vignette_smoothness = 0.5;
    Vector2 vignetteCenter = { 0.5,0.5 };
    Vector3 vignetteColor = { 0,0,0 };

    // FilmGrain
    float grain_intensity = 0.2;
    float grain_response = 0.8;
    float grain_scale = 1;

    // Bloom
    float bloom_threshold = 1.0f;
    float bloom_intensity = 0.8f;
    float bloom_scatter = 0.5f;
    float bloom_clamp = 0.0f;
    Vector3 bloom_tint = { 1.0f, 1.0f, 1.0f };
};


struct FrameWorldData
{
    float   time;
    float   deltaTime;
    Vector2 screenSize;
    Vector2 shadowMapSize = { 8192,8192 };
    Vector3 cameraPos;
};