#pragma once
#include "../pch.h"

/*
    [ Material ]

    SubMesh 단위로 가지며, Material 상수버퍼에 udpate할 데이터입니다.
    TODO :: Script Component에서 사용할 Util Funcs 추가
*/

class Material
{
public:
    // ----- [ Has Flag ] ----- 
    // Texture Flag
    BOOL hasDiffuse = false;
    BOOL hasNormal = false;
    BOOL hasRoughness = false;
    BOOL hasMetallic = false;
    BOOL hasEmissive = false;

    // PBR roughness가 aiTextureType_SHININESS 텍스처로 들어오는 경우
    BOOL roughnessFromShininess = false;


    // ----- [ Values ] ----- 
    // Factor
    Vector3 diffuseFactor   = { 1,1,1 };
    float   alphaFactor     = 1.0f;
    Vector3 emissiveFactor  = { 1,1,1 };
    float   metallicFactor  = 1.0f;
    float   roughnessFactor = 1.0f;

    // Override
    BOOL usediffuseOverride    = false;
    BOOL useEmissiveOverride   = false;
    BOOL useMetallicOverride   = false;
    BOOL useRoughnessOverride  = false;

    Vector3 diffuseOverride    = { 1,1,1 };
    float   metallicOverride   = 1.0f;
    Vector3 emissiveOverride   = { 1,1,1 };
    float   roughnessOverride  = 1.0f;

public:
    // TODO :: Script Component에서 사용 가능한 Util Funcs 추가
};

