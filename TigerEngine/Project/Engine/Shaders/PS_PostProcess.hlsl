// [ PostProcess PixelShader ]
// 렌더링 후처리 단계

// [Post Process]
// 노출 + ToneMapping
//  - Color Adjustments (채도, 대비, Hue Shift, Tint)
//  - Bloom
//  - Film Grain
//  - Vignette
//  - Lift, Gamma, Gain
//  - White Balance (온도, 색조)

#include <shared.fxh>
#include <PBR_Common.fxh>
#include <PostFxCommon.fxh>

// --- Texture Bind Slot ------------------
Texture2D sceneHDR : register(t14);
Texture2D bloomFinal : register(t16);

// --- Sampler Bind Slot ------------------
SamplerState samplerLinear : register(s0);


float4 main(PS_FullScreen_Input input) : SV_TARGET
{
    float2 uv = input.position.xy / screenSize;
    float2 sampleUV = uv;
    
    // HDR sample
    float3 hdr = sceneHDR.Sample(samplerLinear, sampleUV).rgb;

    // Exposure
    float exposureScale = pow(2.0, exposure);
    hdr *= exposureScale;
    
    // Bloom
    if (useBloom)
    {
        float3 bloom = bloomFinal.Sample(samplerLinear, uv).rgb;
        hdr += bloom * bloom_intensity;
    }

    // tone Mapping
    float3 mapped = ACESFilm(hdr);

    // [ PostProcess ] ------------------------------
    float3 colorGrade = mapped;

    // White Balance
    if (useWhiteBalance)
        colorGrade = ApplyWhiteBalance(colorGrade);
      
    // LGG
    if (useLGG)
        colorGrade = ApplyLGG(colorGrade);
    
    // Color Adjustments
    if (useColorAdjustments)
        colorGrade = ApplyColorAdjustments(colorGrade);

    // Film Grain
    if (useFilmGrain)
        colorGrade = ApplyFilmGrain(uv, colorGrade);
    
    // [ Vinette ] --------------------------------
    if (useVignette)
        colorGrade = ApplyVignette(uv, colorGrade);
    
    
    // [ Defalut Gamma ] --------------------------
    float3 finalColor = colorGrade;
    if (useDefaultGamma)
        finalColor = LinearToSRGB(finalColor);

    return float4(finalColor, 1.0f);
}