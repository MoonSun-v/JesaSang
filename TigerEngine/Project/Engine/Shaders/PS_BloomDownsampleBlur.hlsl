/*
    [ Bloom Downsample Blur Pixel Shader ]
    
    Bloom 처리 2단계.
    해상도를 줄이며(Downsample) 여러 mip을 만들고, 블러 처리한다.
    
    하나의 텍스처를 동시에 read/write 할 수 없기 때문에 BloomA, BloomB 두 텍스처를 
    번갈아 사용하는 ping-pong 방식을 사용한다.

    - mip0(가장 큰 해상도) : 선명하고 작은 퍼짐
    - mipN(가장 작은 해상도) : 넓고 흐린 퍼짐

    - Input : Bloom mip(i-1)
    - Output : Bloom mip(i)
*/ 

#include <shared.fxh>

// --- Texture Bind Slot ------------------
Texture2D bloomSrc : register(t16); //  C++에서 A 또는 B를 바인딩 (ping-pong)

// --- Sampler Bind Slot ------------------
SamplerState samLinearClamp : register(s2);


float3 SampleSrc(float2 uv)
{
    return bloomSrc.SampleLevel(samLinearClamp, uv, srcMip).rgb;
}


float4 main(PS_FullScreen_Input input) : SV_TARGET
{
    float2 uv = input.uv;
    float2 t = srcTexelSize;

    // 가벼운 9-tap (근사 가우시안)
    float3 c = 0;
    c += SampleSrc(uv + t * float2(-1, -1));
    c += SampleSrc(uv + t * float2(0, -1)) * 2.0;
    c += SampleSrc(uv + t * float2(1, -1));

    c += SampleSrc(uv + t * float2(-1, 0)) * 2.0;
    c += SampleSrc(uv) * 4.0;
    c += SampleSrc(uv + t * float2(1, 0)) * 2.0;

    c += SampleSrc(uv + t * float2(-1, 1));
    c += SampleSrc(uv + t * float2(0, 1)) * 2.0;
    c += SampleSrc(uv + t * float2(1, 1));

    c *= (1.0 / 16.0);

    return float4(c, 1);

}