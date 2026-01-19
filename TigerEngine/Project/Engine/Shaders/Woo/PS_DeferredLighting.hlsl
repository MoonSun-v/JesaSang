/*
    [ PBR Lighting Pass Pixel Shader ] 
    * Deferred Rendering *

    Full Screen Quad를 그리며, G-buffer에 기록된 정보를 활용하여
    최종 가시 픽셀에 대해서만 라이팅 연산을 진행합니다.

    - Direct BRDF(Cook-Torrance)
    - Indirect IBL(BRDF)
    - Shadow Mapping Support
*/

#include <shared.fxh>
#include <PBR_Common.fxh>

// --- Texture Bind Slot ------------------
Texture2D shadowMap : register(t5);
TextureCube IBL_IrradianceMap : register(t11);
TextureCube IBL_SpecularEnvMap : register(t12);
Texture2D IBL_BRDF_LUT : register(t13);

// Gbuffer Textures
Texture2D albedoTex : register(t6);
Texture2D normalTex : register(t7);
Texture2D metalRoughTex : register(t8);
Texture2D emissiveTex : register(t9);
Texture2D depthTex : register(t10);


// --- Sampler Bind Slot ------------------
SamplerState samLinear : register(s0);
SamplerComparisonState samShadow : register(s1);
SamplerState samLinearClamp : register(s2);


float4 main(PS_FullScreen_Input input) : SV_TARGET
{
    // UV
    float2 uv = input.position.xy / screenSize;
    
    // --- [ World Position ]  ----------------------------------
    float depth = depthTex.Sample(samLinearClamp, uv).r;

    if (depth >= 0.999999f)
        return 0; // 배경 픽셀 스킵
    
    float4 clip;
    clip.x = uv.x * 2.0f - 1.0f;
    clip.y = (1.0f - uv.y) * 2.0f - 1.0f;
    clip.z = depth;
    clip.w = 1.0f;

    float4 worldH = mul(clip, invViewProjection);
    float3 worldPos = worldH.xyz / worldH.w;

    
    // --- [ Read Gbuffer ]  ----------------------------------
    float2 samUV = uv;
    
    float4 base_sample = albedoTex.Sample(samLinearClamp, samUV);
    float3 base_color = base_sample.rgb;
    float alpha = base_sample.a;
    
    float3 N = normalTex.Sample(samLinearClamp, samUV);
    N = DecodeNormal(N);
    
    float2 matalrough = metalRoughTex.Sample(samLinearClamp, samUV).rg;
    float metallic = matalrough.x;
    float roughness = matalrough.y;
    
    float3 emissive = emissiveTex.Sample(samLinearClamp, samUV).rgb;
    
    
    
    // --- [ ShadowMapping ] ------------------------------------
    float shadowFactor = 1.0f;
    
    // 하나의 SunLight에 대해서만 Shadow
    if (isSunLight)
    {
        float4 posShadow = mul(float4(worldPos, 1), shadowView);
        posShadow = mul(posShadow, shadowProjection);
    
        float currentShadowDepth = posShadow.z / posShadow.w;
        float2 uv = posShadow.xy / posShadow.w;
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
            
            float2 texelSize = 1.0 / shadowMapSize; // 텍셀 크기 (ShadowMap 해상도 기준)
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
    }
    
    
    
    
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
        float3 toLight = lightPos - worldPos;
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
        float3 toLight = lightPos - worldPos;
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

    float3 V = normalize(cameraPos - worldPos);
    float3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    
    
    
    // --- [Direct Light]  ------------------------------------
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
    if (useIBL && isSunLight)
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
