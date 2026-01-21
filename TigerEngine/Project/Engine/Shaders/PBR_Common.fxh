#include <shared.fxh>

#ifndef PBRCommon
#define PBRCommon

// --- Cook -Torrance BRDF Functions ------
static const float PI = 3.141592f;
static const float EPSILON = 0.00001f;

// D (Normal Distribution Function)
float D_NDFGGXTR(float3 N, float3 H, float roughness)
{
    float NdotH = max(dot(N, H), 0.0);
    float alpha = roughness * roughness;
    float lower = (NdotH * NdotH) * (alpha - 1.0) + 1.0;
    
    return alpha / max(EPSILON, PI * (lower * lower));
}

// F (Fresnel reflection)
float3 F_Schlick(float3 H, float3 V, float3 F0)
{
    float HdotV = max(dot(H, V), 0.0);
    return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
}

// GSub (SchlickGGX)
float G_SchlickGGX(float3 N, float3 V, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    return NdotV / (NdotV * (1.0 - k) + k);
}

// G (Geometric Attenuation Function)
float G_Smith(float3 N, float3 V, float3 L, float roughness)
{
    float alpha = roughness * roughness;
    float k = pow((alpha + 1), 2) / 8.0; // directinoal light

    return G_SchlickGGX(N, V, k) * G_SchlickGGX(N, L, k);
}


// --- Defalut Gamma ------------------------------
float3 LinearToSRGB(float3 c)
{
    return pow(c, 1.0 / defalutGamma);
}


#endif