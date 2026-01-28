// [ Decal Pixel Shader ]
// Decal Volume Box의 Local UV 좌표를 기반으로 Decal Texture를 샘플링하여 출력합니다.
// - Texture mapping
// - Ring Effect (TODO)

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
    float2 uvScreen = input.pos.xy / screenSize;
    float depth = depthTex.Sample(samLinearClamp, uvScreen).r;

    float4 clip;
    clip.x = uvScreen.x * 2.0f - 1.0f;
    clip.y = (1.0f - uvScreen.y) * 2.0f - 1.0f;
    clip.z = depth;
    clip.w = 1.0f;
    
    float4 worldH = mul(clip, invViewProjection);
    float3 worldPos = worldH.xyz / worldH.w;

    float3 local = mul(float4(worldPos, 1.0f), decalInvWorld).xyz;

    // local mesh vertex [-h, +h] (하드코딩)
    const float h = 10.0f; 

    // inside test
    if (any(abs(local) > h))
        discard;

    // local.xz [-h, +h] -> [0, 1]
    float2 uvDecal = (local.xz / (2.0f * h)) + 0.5f;
    uvDecal = uvDecal * tiling + offset;

    float4 c = decalTex.Sample(samLinearClamp, uvDecal);
    c.a *= opacity;
    if (c.a <= 0.001f)
        discard;

    return c;
}