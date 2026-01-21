// [ PS_Effect ]

#include <shared.fxh>

Texture2D effectTex : register(t18);
SamplerState samLinear : register(s0);

float4 main(PS_Particle_INPUT input) : SV_TARGET
{
    float4 tex = effectTex.Sample(samLinear, input.uv);
    float4 outCol = tex * input.color;

    // alpha cut
    clip(outCol.a - 0.001);

    float4 final = outCol;
    return final;
}