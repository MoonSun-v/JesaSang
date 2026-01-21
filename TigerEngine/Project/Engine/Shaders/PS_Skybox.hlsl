// [ PS_Skybox ]

#include <PBR_Common.fxh>

// --- Texture Bind Slot ------------------
TextureCube skyboxTex : register(t15);

// --- Sampler Bind Slot ------------------
SamplerState samplerLinear : register(s0);


float4 main(PS_Skybox_INPUT input) : SV_TARGET
{
    float3 color = skyboxTex.Sample(samplerLinear, normalize(input.texCoord));
    
    // LDR 단독패스일 때만 감마보정
    if (useDefaultGamma && !isHDR)
        color = LinearToSRGB(color);
    
    return float4(color, 1);
}