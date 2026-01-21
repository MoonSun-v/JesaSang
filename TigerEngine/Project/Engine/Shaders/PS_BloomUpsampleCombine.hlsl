/*
    [ Bloom Upsample Combine Pixel Shader ]
    
    Bloom 처리 3단계.
    mipN(가장 작은 해상도, 넓은 퍼짐)부터 mip0(큰 해상도, 날카로운 코어)로 올라오며 
    업샘플하고 가산 합성하여 최종 Bloom 텍스처를 만든다.
    
    - Input : BloomA mipN
    - Output : BloomFinal  (BloomA mip0 또는 BloomB mip0)
*/

#include <shared.fxh>

// --- Texture Bind Slot ------------------
Texture2D bloomBig : register(t16); // big 해상도 mip 체인 (base)
Texture2D bloomSmall : register(t17); // small 해상도 mip 체인 (업샘플 소스)

// --- Sampler Bind Slot ------------------
SamplerState samLinearClamp : register(s2);


float4 main(PS_FullScreen_Input input) : SV_TARGET
{
    float2 uv = input.uv;
    float3 big = bloomBig.SampleLevel(samLinearClamp, uv, srcMip).rgb;
    float3 small = bloomSmall.SampleLevel(samLinearClamp, uv, srcMip + 1).rgb; // bilinear 업샘플

    // scatter
    float w = lerp(0.5, 1.0, saturate(bloom_scatter));

    // combine (합산)
    float3 combined = big + small * w;
    
    return float4(combined, 1);
}