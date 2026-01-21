#pragma once
#include <DirectXMath.h>
#include <directxtk/simplemath.h>
using namespace DirectX::SimpleMath;


////////////////////////////////////////////
////////    [ ConstantBuffer ]     /////////
////////////////////////////////////////////

// cbuffer FrameCB : register(b0)
// cbuffer TransformCB : register(b1)
// cbuffer LightingCB : register(b2)
// cbuffer MaterialCB : register(b3)
// cbuffer OffsetMatrixCB : register(b4)
// cbuffer PoseMatrixCB : register(b5)
// cbuffer PostProcessCB : register(b6)
// cbuffer BloomCB : register(b7)
// cbuffer EffectCB : register(b8)

// Frame CB -> b0
struct alignas(16) FrameCB
{
    float    time;
    float    deltaTime;
    Vector2  padding1;

    Vector2 screenSize;
    Vector2 shadowMapSize = { 8192,8192 };      // 이렇게 생성 고정함

    Vector3 cameraPos;
    int padding2;
};


// Transform -> b1
struct alignas(16) TransformCB
{
    Matrix model;
    Matrix world;

    int isRigid;       // 1 : rigid, 0 : skinned
    int refBoneIndex;  // 리지드일 때 참조하는 본 인덱스
    Vector2 pad;

    Matrix view;
    Matrix projection;
    Matrix invViewProjection;  // Deferred 화면 좌표 -> view 좌표 변환용

    Matrix shadowView;         // 광원 view
    Matrix shadowProjection;   // 광원 projection
};

// LightingCB -> b2
struct alignas(16) LightingCB
{
    int     lightType;         // 0: Directional, 1: Point, 2: Spot
    Vector3 padding;

    BOOL    isSunLight = 0;    // Directional Light가 태양광인지 여부 (그림자 처리)
    Vector3 lightColor;

    float   directIntensity;
    Vector3 lightDirection;

    Vector3 lightPos;
    float   lightRange;

    float innerAngle;
    float outerAngle;

    float indirectIntensity = 0.2f;
    BOOL  useIBL = 0;
};

// MaterialCB -> b3
struct alignas(16) MaterialCB
{
    // use texture
    BOOL useDiffuse;
    BOOL useNormal;
    BOOL useEmissive;
    BOOL useMetallic;
    BOOL useRoughness;

    BOOL roughnessFromShininess; // roughness가 aiTextureType_SHININESS로 들어오는 경우
    Vector2  padding1;

    // PBR Factor
    float emissiveFactor = 1.0f;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float padding2;

    // PBR override
    BOOL usediffuseOverride = false;
    BOOL useEmissiveOverride = false;
    BOOL useMetallicOverride = false;
    BOOL useRoughnessOverride = false;

    Vector3 diffuseOverride = { 1,1,1 };
    float metallicOverride = 1.0f;
    Vector3 emissiveOverride = { 1,1,1 };
    float roughnessOverride = 1.0f;
};

// OffsetMatrix -> b4
struct alignas(16) OffsetMatrixCB
{
    // bone offset matrix
    // vertex데이터에 참조할 index 4개가 들어있음
    Matrix boneOffset[128];
};

// PoseMatrix -> b5
struct alignas(16) PoseMatrixCB
{
    // bone world matrix
    // bone의 local matrix(animation)을 계층 구조에 따라 누적한 bone transform 배열
    // vertex데이터에 참조할 index 4개가 들어있음
    Matrix bonePose[128];
};

// PostProcess CB -> b6
struct alignas(16) PostProcessCB
{
    // Base
    BOOL    isHDR = true;               // LDR/HDR
    BOOL    useDefaultGamma = true;     // Linear -> SRGB
    float   defaultGamma = 2.2f;     // Gamma (defalut)
    float   exposure = 0;            // 노출

    // Enable
    BOOL useColorAdjustments = false;
    BOOL useWhiteBalance = false;
    BOOL useLGG = false;
    BOOL useVignette = false;
    BOOL useFilmGrain = false;
    BOOL useBloom = false;
    Vector2 padding0;

    // Color Adjustments (대비, 채도, Hue Shift, Tint)
    float   contrast = 1;
    float   saturation = 1;
    BOOL    useHueShift = false;
    float   hueShift = 0;

    BOOL    useColorTint = false;
    Vector3 colorTint = { 1,1,1 };
    float   colorTint_strength = 0.5;
    Vector3 padding2;

    // White Balance (온도, 색조)
    float temperature = 0;
    float tint = 0;
    Vector2 padding3;

    // Lift / Gamma / Gain (어두운톤, 미드톤, 밝은톤 밝기 조정)
    BOOL  useLift = false;
    BOOL  useGamma = false;
    BOOL  useGain = false;
    int   padding4;

    Vector3 lift = { 0,0,0 };
    float   lift_strength = 0.5;
    Vector3 gamma = { 0,0,0 };
    float   gamma_strength = 1.0;
    Vector3 gain = { 0,0,0 };
    float   gain_strength = 0.5;

    // Vinette
    float   vignette_intensity = 0.5;
    float   vignette_smoothness = 0.5;
    Vector2 vignetteCenter = { 0.5,0.5 };
    Vector3 vignetteColor = { 0,0,0 };
    int     padding5;

    // FilmGrain
    float grain_intensity = 0.2;
    float grain_response = 0.8;
    float grain_scale = 1;
    int   padding6;
};

// Bloom CB -> b7
struct alignas(16) BloomCB
{
    float bloom_threshold = 1.0f;
    float bloom_intensity = 0.8f;
    float bloom_scatter = 0.5f;
    float bloom_clamp = 0.0f;

    Vector3 bloom_tint = { 1.0f, 1.0f, 1.0f };
    int     padding;

    int     srcMip = 0;                      // SampleLevel용 mip 인덱스
    Vector2 srcTexelSize = { 0.0f, 0.0f };   // 패스에서 읽고 있는 mip 레벨의 텍스처 해상도를 기준으로 한 texel size
    int     padding2;
};

// Effect CB -> b8
struct alignas(16) EffectCB
{
    Vector2 atlasGrid;      // (cols, rows)
    Vector2 invAtlasGrid;   // (1/cols, 1/rows)

    float baseSizeScale = 1.0f;
    int billboardType;
    Vector2 padding;
};