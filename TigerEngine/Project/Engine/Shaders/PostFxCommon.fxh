#include <shared.fxh>

#ifndef POSTFxCommon
#define POSTFxCommon


// Hash Noise
float Hash12(float2 p)
{
    float h = dot(p, float2(127.1, 311.7));
    return frac(sin(h) * 43758.5453);
}


// ----------------------------------------
// [ Post Process ] 
// ----------------------------------------
// Color Adjustments
float3 ApplyColorAdjustments(float3 color)
{
    // 색상 이동
    if (useHueShift)
    {
        // RGB -> YIQ
        float Y = dot(color, float3(0.299, 0.587, 0.114));
        float I = dot(color, float3(0.596, -0.274, -0.322));
        float Q = dot(color, float3(0.211, -0.523, 0.312));

        float cosA = cos(hueShift);
        float sinA = sin(hueShift);

        float I2 = I * cosA - Q * sinA;
        float Q2 = I * sinA + Q * cosA;

        // YIQ -> RGB
        color.r = Y + 0.956 * I2 + 0.621 * Q2;
        color.g = Y - 0.272 * I2 - 0.647 * Q2;
        color.b = Y - 1.106 * I2 + 1.703 * Q2;
    }

    // 대비
    color = ((color - 0.5) * contrast + 0.5);

    // 채도
    float gray = dot(color, float3(0.299, 0.587, 0.114));
    color = lerp(float3(gray, gray, gray), color, saturation);

    // 톤
    if (useTint)
        color = lerp(color, color * colorTint, colorTint_strength);

    return saturate(color);
}

// Film Grain
float3 ApplyFilmGrain(float2 uv, float3 color)
{
    // lumma (밝기)
    float lum = dot(color, float3(0.2126, 0.7152, 0.0722));

    // response 0(shadow 강조) ~ 1(highlight 강조)
    float resp = saturate(grain_response);
    float weight = lerp(1.0 - lum, lum, resp);
    weight = pow(saturate(weight), 0.75);

    // pixel 좌표 기반
    float2 screenTexelSize = 1 / screenSize;
    float2 pixel = uv / max(screenTexelSize, 1e-8.xx);
    float2 p = pixel * max(grain_scale, 1e-3);

    // noise
    float n = Hash12(p) * 2.0 - 1.0;
    float amt = saturate(grain_intensity) * weight;

    // 모노 그레인(additive)
    return saturate(color + n * amt);
}

// Vinette
float3 ApplyVignette(float2 uv, float3 color)
{
    // 화면 종횡비 보정
    // screenTexelSize = (1/width, 1/height)
    float2 screenTexelSize = 1 / screenSize;
    float aspect = screenTexelSize.y / max(screenTexelSize.x, 1e-8);
    float2 p = uv - vignetteCenter;
    p.x *= aspect;

    // 중심에서 코너까지의 최대 거리로 정규화 0(center) ~ 1(corner)
    float2 corner = float2(0.5 * aspect, 0.5);
    float rMax = max(length(corner), 1e-6);
    float d = length(p) / rMax;

    // intensity : 어두운 영역 범위(inner)
    // smoothness : 부드러운 경계(outer)
    float intensity = saturate(vignette_intensity);
    float smoothness = saturate(vignette_smoothness);
    
    float inner = saturate(1.0 - intensity);
    float outer = saturate(inner + max(smoothness, 1e-4));
    float mask = smoothstep(inner, outer, d);

    // vinette color
    float t = mask * intensity;
    return lerp(color, vignetteColor, t);
    
    return color;
}

// Lift/ gamma/ Gain
float3 ApplyLGG(float3 color)
{
    // Lift (shadows offset)
    if (useLift)
    {
        color += lift * lift_strength;
        color = max(color, 0.0);
    }

    // Gamma (midtones curve)
    if (useGamma)
    {
        // float3 v = {1,1,1} == 1.0.xxx
        float3 gammaPower = lerp(1.0.xxx, 1.0.xxx + gamma, gamma_strength);
        gammaPower = max(gammaPower, 0.05.xxx); // 폭발 방지
        color = pow(max(color, 1e-4.xxx), 1.0 / gammaPower);
    }

    // Gain (highlights scale)
    if (useGain)
    {
        float3 gainScale = lerp(1.0.xxx, 1.0.xxx + gain, gain_strength);
        gainScale = max(gainScale, 0.0.xxx); // 음수 방지
        color *= gainScale;
    }

    return color;
}

// White Balance
float3 ApplyWhiteBalance(float3 color)
{
    // Temperature : Blue <-> Yellow
    float3 tempOffset = float3(0.10, 0.0, -0.10) * temperature;

    // Tint : Magenta <-> Green 
    float3 tintOffset = float3(-0.05, 0.10, -0.05) * tint;

    color += tempOffset + tintOffset;
    return color;
}


// ----------------------------------------
// ACES Filmic Tone Mapping : HDR -> LDR 압축 (0~1)
// ----------------------------------------
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate(x * (a * x + b) / (x * (c * x + d) + e));
}

#endif