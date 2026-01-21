/*
    [ Bloom Prefilter Pixel Shader ]
    
    Bloom 처리 1단계.
    sceneHDR 텍스처에서 Bloom처리할 밝은 부분을 추출한다.
    
    - Input : sceneHDR
    - Output : BloomA mip0
*/

#include <shared.fxh>

// --- Texture Bind Slot ------------------
Texture2D sceneHDR : register(t14);

// --- Sampler Bind Slot ------------------
SamplerState samLinearClamp : register(s2);


float Luminance(float3 c)
{
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

// soft-knee
float PrefilterMask(float lum, float threshold, float knee01)
{
    float knee = max(1e-4, knee01) * threshold;
    float x = lum - (threshold - knee);
    float y = saturate(x / (2.0 * knee));
    float soft = y * y * (3.0 - 2.0 * y);
    return soft * step(threshold - knee, lum);
}

float4 main(PS_FullScreen_Input input) : SV_TARGET
{
    float2 uv = input.uv;
    float3 hdr = sceneHDR.Sample(samLinearClamp, input.uv).rgb;

    // 너무 밝은값 clamp
    if (bloom_clamp > 0.0f)
        hdr = min(hdr, bloom_clamp.xxx);

    float lum = Luminance(hdr);
    float m = PrefilterMask(lum, bloom_threshold, bloom_scatter);

    // tint
    float3 outcolor = hdr * m * bloom_tint;

    return float4(outcolor, 1);
}