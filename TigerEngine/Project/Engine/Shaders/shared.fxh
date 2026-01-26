#ifndef Shared
#define Shared

// [ ConstantBuffer ]
// cbuffer FrameCB : register(b0)
// cbuffer TransformCB : register(b1)
// cbuffer LightingCB : register(b2)
// cbuffer MaterialCB : register(b3)
// cbuffer OffsetMatrixCB : register(b4)
// cbuffer PoseMatrixCB : register(b5)
// cbuffer PostProcessCB : register(b6)
// cbuffer BloomCB : register(b7)
// cbuffer EffectCB : register(b8)


// [ Texture ]
// Texture2D diffuseMap          : register(t0);
// Texture2D normalMap           : register(t1);
// Texture2D metallicMap         : register(t2);
// Texture2D roughnessMap        : register(t3);
// Texture2D emissiveMap         : register(t4);
// Texture2D shadowMap           : register(t5);

// Texture2D albedoTex           : register(t6);
// Texture2D normalTex           : register(t7);
// Texture2D metalRoughTex       : register(t8);
// Texture2D emissiveTex         : register(t9);
// Texture2D depthTex            : register(t10);

// Texture2D IBL_IrradianceMap   : register(t11);
// Texture2D IBL_SpecularEnvMap  : register(t12);
// Texture2D IBL_BRDF_LUT        : register(t13);

// Texture2D sceneHDR            : register(t14);
// TextureCube skyboxTex         : register(t15);
// Texture2D bloomATex           : register(t16);
// Texture2D bloomBTex           : register(t17);
// Texture2D effectTex           : register(t18);


// [ SamplerState ]
// SamplerState samLinear           : register(s0);
// SamplerComparisonState samShadow : register(s1);
// SamplerState samLinearClamp      : register(s2);


// ------------------
//  Constant Buffer
// ------------------
cbuffer FrameCB : register(b0)
{
    float time;
    float deltaTime;
    float2 padding15;

    float2 screenSize;
    float2 shadowMapSize;

    float3 cameraPos;
    int padding16;
}

cbuffer TransformCB : register(b1)
{
    matrix model; 
    matrix world;

    matrix view;
    matrix projection;
    matrix invViewProjection;
    
    matrix shadowView;
    matrix shadowProjection;
}

cbuffer LightingCB : register(b2)
{
    int lightType;
    float3 padding;
    
    bool isSunLight;
    float3 lightColor;
    
    float directIntensity;
    float3 lightDirection;
    
    float3 lightPos;
    float lightRange;
    
    float innerAngle;
    float outerAngle;
    
    float indirectIntensity;
    bool useIBL;
}

cbuffer MaterialCB : register(b3)
{
    // use texture
    bool useDiffuse;
    bool useNormal;
    bool useEmissive;
    bool useMetallic;
    bool useRoughness;
    bool roughnessFromShininess;
    float2 padding3;
    
    // PBR Factor
    float3 diffuseFactor;
    float  alphaFactor;
    float3 emissiveFactor;
    float metallicFactor;
    float roughnessFactor;
    float3 padding4;

    // PBR override
    bool useBaseColorOverride;
    bool useEmissiveOverride;
    bool useMetallicOverride;
    bool useRoughnessOverride;

    float3 baseColorOverride;
    float metallicOverride;
    float3 emissiveOverride;
    float roughnessOverride;
}

cbuffer OffsetMatrixCB : register(b4)
{
    matrix boneOffset[128];
}

cbuffer PoseMatrixCB : register(b5)
{
    matrix bonePose[128];
}

cbuffer PostProcessCB : register(b6)
{
    // Base
    bool isHDR;
    bool useDefaultGamma;
    float defalutGamma;
    float exposure;
    
    // Enable
    bool useColorAdjustments;
    bool useWhiteBalance;
    bool useLGG;
    bool useVignette;
    bool useFilmGrain;
    bool useBloom;
    float2 padding01;
    
    // Color Adjustments (대비, 채도, Hue Shift, Tint)
    float contrast;
    float saturation;
    bool useHueShift;
    float hueShift;
    
    bool useTint;
    float3 colorTint;
    float colorTint_strength;
    float3 padding6;
    
    // White Balance (온도, 색조)
    float temperature;
    float tint;
    float2 padding7;
    
    // Lift / Gamma / Gain
    bool useLift;
    bool useGamma;
    bool useGain;
    float padding8;
    
    float3 lift;
    float lift_strength;

    float3 gamma;
    float gamma_strength;

    float3 gain;
    float gain_strength;
    
    // Vinette
    float vignette_intensity;
    float vignette_smoothness;
    float2 vignetteCenter;
    float3 vignetteColor;
    int padding9;
    
    // FilmGrain
    float grain_intensity;
    float grain_response;
    float grain_scale;
    int padding10;
};

cbuffer BloomCB : register(b7)
{
    float bloom_threshold;
    float bloom_intensity;
    float bloom_scatter;
    float bloom_clamp;

    float3 bloom_tint;
    int padding13;
    
    int srcMip;
    float2 srcTexelSize;
    int padding14;
}

cbuffer EffectCB : register(b8)
{
    float2 atlasGrid;
    float2 invAtlasGrid;

    float baseSizeScale;
    int billboardType;
    float2 padding17;
}

cbuffer PickingCB : register(b9)
{
    uint PickingID;
    float3 pad;
}


// ----------------------
//  Vertex Input Layout
// ----------------------
struct VS_Rigid_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float2 texcoord : TEXCOORD;
};

struct VS_Weight_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float2 texCoord : TEXCOORD;
    uint4 boneIndices : BONE_INDICES;
    float4 boneWeights : BONE_WEIGHTS;
};

struct VS_Position_INPUT
{
    float3 position : POSITION;
};

struct VS_Particle_INPUT
{
    // Vertex (ParticleQuadVertex)
    float2 corner : POSITION;       // slot 0
    float2 uv : TEXCOORD0;          // slot 0
    
    // Instance (ParticleInstance)
    float3 pos : TEXCOORD1;         // slot 1
    float rotation : TEXCOORD2;     // slot 1
    float2 size : TEXCOORD3;        // slot 1
    float frame : TEXCOORD4;        // slot 1
    float4 color : COLOR0;          // slot 1
};

// ----------------------
//  PS Input (VS Output)
// ----------------------
struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 worldPos : WORLD_POSITION;
    float3 normal : NORMAL;
    float3x3 TBN : TBN;
    float2 texCoord : TEXCOORD;
    matrix finalWorld : FINAL_WORLD;
    float4 posShadow : TEXCOORD1;
};

struct PS_OutLine_INPUT
{
    float4 pos : SV_POSITION;
};

struct PS_Skybox_INPUT
{
    float4 position : SV_POSITION;
    float3 texCoord : TEXCOORD0; // CubeMap 샘플용 방향 벡터
};

struct PS_FullScreen_Input
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PS_Particle_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

// ------------------------------------
//  PS Output (Deferred Rendering)
// ------------------------------------
struct PS_Output
{
    //float4 WorldPos   : SV_Target0;       // Position 대신 Depth Buffer 사용
    float4 Base_color : SV_Target0;
    float4 Normal : SV_Target1;
    float4 Material : SV_Target2;
    float4 Emissive : SV_Target3;
};


// Deferred Rendering Normal Gbuffer 
float3 DecodeNormal(float3 enc)
{
    // enc is in [0,1], decode to [-1,1]
    return normalize(enc * 2.0f - 1.0f);
}

float3 EncodeNormal(float3 n)
{
    // n is in [-1,1], encode to [0,1]
    return n * 0.5f + 0.5f;
}

#endif