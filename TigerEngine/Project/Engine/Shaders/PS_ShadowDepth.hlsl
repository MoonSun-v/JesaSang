// [ PS_ShadowDepth ]
// 알파 픽셀을 ShadowMap에 기록하지 않기 위한 Pixel Shader
// 필요 없다면 Depth Only Pass는 pixel shader 실행 x

#include <shared.fxh>

// --- Texture Bind Slot ------------------
Texture2D diffuseMap : register(t0);

// --- Sampler Bind Slot ------------------
SamplerState samLinear : register(s0);


void main(PS_INPUT input)
{
    float alpha = 1.0f;
    if (useDiffuse)
        alpha = diffuseMap.Sample(samLinear, input.texCoord).a;
    
    clip(alpha - 0.5f); // discard나 clip이 있다면 Early z test 진행 x
}