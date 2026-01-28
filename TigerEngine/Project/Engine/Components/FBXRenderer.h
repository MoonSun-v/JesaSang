#pragma once
#include <pch.h>
#include "../Components/RenderComponent.h"
#include "../Components/FBXData.h"
#include "Datas/FBXResourceData.h"
#include "Datas/Bone.h"

class FBXRenderer : public RenderComponent
{
	RTTR_ENABLE(RenderComponent)

public:
    // [Component Process] -----------------
	void OnInitialize() override;
	void OnStart() override;
	void OnUpdate(float delta) override;
    void OnDestory() override;
    void OnRender(RenderQueue& queue) override;

	
    // [Json Process] -----------------
	nlohmann::json Serialize() override;
	void Deserialize(nlohmann::json data) override;


    // [Material get/set] -----------------
    Color GetDiffuse() { return Color(diffuseFactor.x, diffuseFactor.y, diffuseFactor.z); }
    void SetDiffuse(Color color);
    float GetAlpha() { return alphaFactor; }
    void SetAlpha(float value);
    Color GetEmissive() { return Color(emissiveFactor.x, emissiveFactor.y, emissiveFactor.z); }
    void SetEmissive(Color color);
	float GetRoughness() { return roughnessFactor; }
    void SetRoughness(float value);
	float GetMatalic() { return metalicFactor; }
    void SetMatalic(float value);
	
    bool GetUseDiffuseOverride() { return useDiffuseOverride; }
    void SetUseDiffuseOverride(bool flag);
    bool GetUseEmissiveOverride() { return useEmissiveOverride; }
    void SetUseEmissiveOverride(bool flag);
    bool GetUseRoughnessOverride() { return useRoughnessOverride; }
    void SetUseRoughnessOverride(bool flag);
    bool GetUseMatalicOverride() { return useMetallicOverride; }
    void SetUseMatalicOverride(bool flag);

    Color GetDiffuseOverride() { return Color(diffuseOverride.x, diffuseOverride.y, diffuseOverride.z); }
    void SetDiffuseOverride(Color color);
    Color GetEmissiveOverride() { return Color(emissiveOverride.x, emissiveOverride.y, emissiveOverride.z); }
    void SetEmissiveOverride(Color color);
    float GetMetallicOverride() { return metallicOverride; }
    void SetMetallicOverride(float value);
    float GetRoughnessOverride() { return roughnessOverride; }
    void SetRoughnessOverride(float value);      


    // [RenderType get/set] -----------------
    RenderBlendType GetRenderBlendType() { return renderBlendType; }
    void SetRenderBlendType(RenderBlendType type) { renderBlendType = type; }

    bool GetIsGround() { return isGround; }
    void SetIsGround(bool flag) { isGround = flag; }

    // [Bone] -----------------
    void CreateBoneInfo();
    
private:
    // FBX Asset Data
    FBXData* fbxData = nullptr;   // 참조할 FBX 데이터

	// Instance Data
	std::string directory{};	  // 로드한 파일이 위차한 폴더명

	// 해당 모델의 상수 버퍼 내용
    PoseMatrixCB bonePoses{};

    // Ground Mesh
    bool isGround = false;        // Ground Projection Decal 처리를 위한 flag

    // Material
    Vector3 diffuseFactor   = { 1,1,1 };
    float   alphaFactor     = 1.0f;
    Vector3 emissiveFactor  = { 1,1,1 };
	float   roughnessFactor = 1.0f;
	float   metalicFactor   = 1.0f;

    bool useDiffuseOverride   = false;
    bool useEmissiveOverride  = false;
    bool useMetallicOverride  = false;
    bool useRoughnessOverride = false;

    Vector3 diffuseOverride   = { 1,1,1 };
    Vector3 emissiveOverride  = { 1,1,1 };
    float   metallicOverride  = 1.0f;
    float   roughnessOverride = 1.0f;
};