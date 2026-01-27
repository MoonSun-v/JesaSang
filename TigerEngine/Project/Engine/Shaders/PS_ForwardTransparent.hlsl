/*
    [ PBR Forward Transparent Pixel Shader] 
    * Deferred Rendering *

    Deffered Rendering에서 처리되지 못한 투명 오브젝트들을 Forward Rendering 합니다.

    - Direct BRDF(Cook-Torrance)
    - Indirect IBL(BRDF)
    - Shadow Mapping Support
*/

#include <shared.fxh>
#include <PBR_Common.fxh>

// --- Texture Bind Slot ------------------
Texture2D diffuseMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D metallicMap : register(t2);
Texture2D roughnessMap : register(t3);
Texture2D emissiveMap : register(t4);

Texture2D shadowMap : register(t5);
TextureCube IBL_IrradianceMap : register(t11);
TextureCube IBL_SpecularEnvMap : register(t12);
Texture2D IBL_BRDF_LUT : register(t13);


// --- Sampler Bind Slot ------------------
SamplerState samLinear : register(s0);
SamplerComparisonState samShadow : register(s1);
SamplerState samLinearClamp : register(s2);


float4 main(PS_INPUT input) : SV_TARGET
{
    // --- [Default] ----------------------------------
    // color
    float3 base_color = float3(1.0f, 1.0f, 1.0f);
    float3 emissive = float3(0.0f, 0.0f, 0.0f);
    float metallic = 0.0f;
    float roughness = 0.0f;
    float alpha = 1.0f;
    
    // shadowFactor
    float shadowFactor = 1.0f;
    
    
    // --- [ShadowMapping] ---------------------------
    float currentShadowDepth = input.posShadow.z / input.posShadow.w;
    float2 uv = input.posShadow.xy / input.posShadow.w;
    uv.y = -uv.y;
    uv = uv * 0.5 + 0.5;
    
    if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0)
    {
        float2 offsets[9] =
        {
            float2(-1, -1), float2(0, -1), float2(1, -1),
            float2(-1, 0), float2(0, 0), float2(1, 0),
            float2(-1, 1), float2(0, 1), float2(1, 1)
        };
        float2 texelSize = 1.0 / shadowMapSize;
        shadowFactor = 0.0f;
       
       //  PCF - 9 texel 평균으로 그림자 팩터 계산
       [unroll]
        for (int i = 0; i < 9; i++)
        {
            float2 sampleUV = uv + offsets[i] * texelSize;
            shadowFactor += shadowMap.SampleCmpLevelZero(samShadow, sampleUV, currentShadowDepth - 0.001);
        }
        shadowFactor = shadowFactor / 9.0f;
    }

    
    // --- [Material]  ----------------------------------
    // base_color, alpha
    if (useDiffuse)
    {
        base_color = diffuseMap.Sample(samLinear, input.texCoord).rgb;
        alpha = diffuseMap.Sample(samLinear, input.texCoord).a;
        alpha *= alphaFactor;
    }
    
    if(alpha < 0.05f)
        discard;   // alpha cutoff
    
    // normal
    float3 N;
    if (useNormal)
    {
        float3 local_normal = normalMap.Sample(samLinear, input.texCoord).xyz * 2.0f - 1.0f;
        float3 world_normal = normalize(mul(local_normal, input.TBN));
        N = normalize(world_normal);
    }
    else
    {
        N = normalize(input.worldNormal);
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
    


    // --- [Vector / Attenuation]  ------------------------------------------
    float3 L = 0;
    float attenuation = 1.0f;
    float spotFactor = 1.0f;
    
    // Directional Light
    if (lightType == 0)
    {
        L = normalize(-lightDirection);
    }
    // Point Light
    else if (lightType == 1)
    {
        // defualt
        attenuation = 0.0f;
        L = 0.0f;
        
        // values
        float3 toLight = lightPos - input.worldPos;
        float distSq = dot(toLight, toLight);
        float rangeSq = lightRange * lightRange;

        // 범위
        if (lightRange > 1e-4f && distSq < rangeSq)
        {
            float dist = sqrt(distSq);
            L = toLight / max(dist, 1e-4f);

            // Range Falloff (fade)
            float x = distSq / max(rangeSq, 1e-4f);
            float smooth = saturate(1.0f - x);
            smooth *= smooth;

            // 물리적 거리 감쇠
            float invSq = 1.0f / max(distSq, 1e-4f);

            // 최종 감쇠
            attenuation = smooth * invSq;
        }
    }
    // Spot Light
    else if (lightType == 2)
    {
        // defualt
        attenuation = 0.0f;
        spotFactor = 0.0f;
        L = 0.0f;
        
        // values
        float3 toLight = lightPos - input.worldPos;
        float distSq = dot(toLight, toLight);
        float range = lightRange;
        float rangeSq = range * range;

        // 범위
        if (range > 1e-4f && distSq < rangeSq)
        {
            float dist = sqrt(distSq);
            L = toLight / max(dist, 1e-4f);
            
            // 거리 감쇠
            float x = distSq / max(rangeSq, 1e-4f);
            float smoothRange = saturate(1.0f - x);
            smoothRange *= smoothRange;

            float invSq = 1.0f / max(distSq, 1e-4f);
            float rangeAttenuation = smoothRange * invSq;
            
            // Spot Cone 감쇠
            float3 spotDir = normalize(lightDirection);
            float cosTheta = dot(spotDir, -L);
            float cosInner = cos(radians(innerAngle)); // input은 degree 기준
            float cosOuter = cos(radians(outerAngle));

            // 뒤집힘 방지
            float lo = min(cosOuter, cosInner);
            float hi = max(cosOuter, cosInner);

            float cone = smoothstep(lo, hi, cosTheta);
            cone = pow(cone, 4.0f);
            
            // 최종
            spotFactor = cone;
            float inv = 1.0f / max(dist, 1e-4f); // 1/d
            attenuation = smoothRange * inv * cone;
            
             // 콘 밖이면 사실상 0
            if (spotFactor <= 1e-4f)
            {
                attenuation = 0.0f;
                spotFactor = 0.0f;
            }
        }
    }

    float3 V = normalize(cameraPos - input.worldPos);
    float3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    

    
    
    // --- [Direct Light]  ----------------------------------
    // Specular BRDF (Cook-Torrance)
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), base_color, metallic);
    float D = D_NDFGGXTR(N, H, roughness); // 미세면 정렬정도
    float3 F = F_Schlick(H, V, F0); // 프레넬 반사율
    float G = G_Smith(N, V, L, roughness); // shadowing & masking
    
    float denom = 4.0f * max(NdotL, 0.001) * max(NdotV, 0.001);
    float3 SpecularBRDF = (D * F * G) / denom;

    
    // Diffuse BRDF (Lambertian)
    float3 kd = lerp(1.0 - F, 0.0, metallic); // 표면산란 계수
    float3 DiffuseBRDF = (base_color / PI) * kd;
    
    // Final DirectLight
    float3 DirectColor = (SpecularBRDF + DiffuseBRDF) * lightColor * directIntensity * (NdotL * attenuation * spotFactor);

    
    
    
    
    // --- [Indirect Light]  ----------------------------------
    float3 IndirectColor = { 0, 0, 0 };
    if (useIBL)
    {
        // Diffuse Term --------------------------
        // Irradiance - diffuse BRDF 적분값
        float3 Irradiance = IBL_IrradianceMap.Sample(samLinear, N).rgb;
        float3 DiffuseIBL = base_color * Irradiance * kd;
        
        // Specular Term -----------------------
        uint specularTextureLevels, width, height;
        IBL_SpecularEnvMap.GetDimensions(0, width, height, specularTextureLevels);
        float maxLevel = max(1.0, (float) (specularTextureLevels - 1));
        float mip = saturate(roughness) * maxLevel;
        
        // Prefiltered - 환경 Radiance + D(미세면 분포) + roughness 관련 적분값
        float3 R = normalize(reflect(-V, N));
        float3 PrefilteredColor = IBL_SpecularEnvMap.SampleLevel(samLinear, R, mip).rgb;

        // LUT - F + G 적분값
        float2 BRDF_LUT = IBL_BRDF_LUT.Sample(samLinearClamp, float2(NdotV, roughness)).rg;
        
        // Specular IBL
        float3 SpecularIBL = PrefilteredColor * (F0 * BRDF_LUT.x + BRDF_LUT.y);

        // Final InDirectLight
        IndirectColor = (DiffuseIBL + SpecularIBL) * indirectIntensity;
    }
    
    
    // --- [Final Color]  ----------------------------------
    float3 finalColor = (DirectColor * shadowFactor) + IndirectColor + emissive;

     // LDR 단독패스일 때만 감마보정
    if (useDefaultGamma && !isHDR)
        finalColor = LinearToSRGB(finalColor);

    return float4(finalColor, alpha);
}
