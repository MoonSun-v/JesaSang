// [ Decal Pixel Shader ]
// 


#include <shared.fxh>

// --- Texture Bind Slot ------------------
Texture2D normalTex : register(t7);
Texture2D depthTex : register(t10);
Texture2D decalTex : register(t19);

// --- Sampler Bind Slot ------------------
SamplerState samLinear : register(s0);
SamplerState samLinearClamp : register(s2);


float4 main(PS_Position_INPUT input) : SV_TARGET
{
    // 임시
    float2 uv = float2(0.5f, 0.5f);
    float4 decalColor = decalTex.Sample(samLinear, uv);

    return decalColor;
}