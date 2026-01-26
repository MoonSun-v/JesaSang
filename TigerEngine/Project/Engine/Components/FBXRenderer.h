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
	void OnInitialize() override;
	void OnStart() override;
	void OnUpdate(float delta) override;
    void OnDestory() override;
    void OnRender(RenderQueue& queue) override;
	
	nlohmann::json Serialize() override;
	void Deserialize(nlohmann::json data) override;

	// animation get/set
	int GetAnimationIndex() { return animationIndex; }
	void SetAnimationIndex(int index) { animationIndex = index; }
	
	float GetProgressAnimationTime() { return progressAnimationTime; }	
	void SetProgressAnimationTime(float animTime) { progressAnimationTime = animTime; }

	bool GetIsAnimationPlay() { return isAnimPlay; }
	void SetIsAnimationPlay(bool value) { isAnimPlay = value; }

    // material get/set
    float GetAlpha() { return alphaFactor; }
    void SetAlpha(float value);

    float GetEmissive() { return emissiveFactor; }
    void SetEmissive(float value);
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

    // bone
    void CreateBoneInfo();

private:
    FBXData* fbxData{};                 // 참조할 FBX 데이터

	// 모델 인스턴스 데이터
	std::string directory{};		    // 로드한 파일이 위차한 폴더명
	std::vector<Bone> bones{};			// 로드된 모델의 본 모음

	// 해당 모델의 상수 버퍼 내용
    PoseMatrixCB bonePoses{};

    // animation info
    int animationIndex = 0;             // 현재 실행 중인 애니메이션 인덱스
    float progressAnimationTime = 0.0f; // 현재 애니메이션 시간
    bool isAnimPlay = true;   

    // Material
    float alphaFactor = 1.0f;

    float emissiveFactor = 1.0f;
	float roughnessFactor = 1.0f;
	float metalicFactor = 1.0f;

    bool useDiffuseOverride = false;
    bool useEmissiveOverride = false;
    bool useMetallicOverride = false;
    bool useRoughnessOverride = false;

    Vector3 diffuseOverride = { 1,1,1 };
    Vector3 emissiveOverride = { 1,1,1 };
    float   metallicOverride = 1.0f;
    float   roughnessOverride = 1.0f;
};