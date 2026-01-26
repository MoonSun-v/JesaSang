/*
    [ PBR Geomatry Pass Pixel Shader ] 
    * Deferred Rendering *
    
    MTR을 활용하여 라이팅 연산에 필요한 정보들을 Gbuffer에 기록
     RT0 : Albedo (RGB)
     RT1 : Normal (RGB)
     RT2 : Metallic (R), Roughness (G)
     RT3 : Emissive (RGB)
     -> Position은 depth buffer를 복원하여 사용합니다.
*/

#include <shared.fxh>

// --- Texture Bind Slot ------------------
Texture2D diffuseMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D metallicMap : register(t2);
Texture2D roughnessMap : register(t3);
Texture2D emissiveMap : register(t4);

// --- Sampler Bind Slot ------------------
SamplerState samLinear : register(s0);


PS_Output main(PS_INPUT input) : SV_TARGET
{
    PS_Output output = (PS_Output) 0;
    
    // --- [ Default ] ----------------------------------
    
    float3 base_color = float3(1.0f, 1.0f, 1.0f);
    float3 normal = float3(0, 0, 0);
    float3 emissive = float3(0.0f, 0.0f, 0.0f);
    float metallic = 0.0f;
    float roughness = 0.0f;
    float alpha = 1.0f;
    
    
    // ---[ Material Texture ]----------------------------------
    
    // base color, aplha
    if (useDiffuse)
    {
        base_color = diffuseMap.Sample(samLinear, input.texCoord).rgb;
        alpha = diffuseMap.Sample(samLinear, input.texCoord).a;
        alpha *= alphaFactor;
    }
    
    if (alpha < 0.5)
        discard;
    
    // normal
    if (useNormal)
    {
        float3 local_normal = normalMap.Sample(samLinear, input.texCoord).xyz * 2.0f - 1.0f;
        float3 world_normal = normalize(mul(local_normal, input.TBN));
        normal = normalize(world_normal);
    }
    else
    {
        normal = normalize(input.normal);
    }
    
    // emission
    if (useEmissive)
        emissive = emissiveMap.Sample(samLinear, input.texCoord).rgb;
    
    // metallic
    if (useMetallic)
        metallic = metallicMap.Sample(samLinear, input.texCoord).r;

    // roughness
    if (useRoughness)
    {
        roughness = roughnessMap.Sample(samLinear, input.texCoord).r;
        if (roughnessFromShininess)
            roughness = 1 - roughness;
    }
    
    
    // --- [Override] ----------------------------------
    
    if (useBaseColorOverride)
        base_color = baseColorOverride;
    if (useEmissiveOverride)
        emissive = emissiveOverride;
    if (useMetallicOverride)
        metallic = metallicOverride;
    if (useRoughnessOverride)
        roughness = roughnessOverride;
    
    roughness = max(roughness, 0.04);
    
    // --- [Factor] -----------------------------------
    
    base_color *= diffuseFactor;
    emissive *= emissiveFactor;
    metallic *= metallicFactor;
    float rf = max(roughnessFactor, 0.04);
    roughness *= rf;


    // ---[ Write G-Buffer ]----------------------------------

    output.Base_color = float4(base_color, alpha);
    output.Normal = float4(EncodeNormal(normal), 1.0f); // -1,0,1 => 0,1
    output.Material = float4(metallic, roughness, 1.0f, 1.0f);
    output.Emissive = float4(emissive, 1.0f);
    

    return output;
}
