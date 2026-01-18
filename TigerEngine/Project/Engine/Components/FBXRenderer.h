#pragma once
#include <pch.h>
#include "Entity/RenderComponent.h"
#include "../Components/FBXData.h"
#include "Datas/FBXResourceData.h"
#include "Datas/Bone.h"
#include "../Commands/DrawFBXCommand.h"

class FBXRenderer : public RenderComponent
{
	RTTR_ENABLE(Component)
public:
	void OnInitialize() override;
	void OnStart() override;
	void OnUpdate(float delta) override;
    void OnDestory() override;
    void OnRender(ComPtr<ID3D11DeviceContext>& context) override;
	
	nlohmann::json Serialize() override;
	void Deserialize(nlohmann::json data) override;

	// animation
	int GetAnimationIndex() { return animationIndex; }
	void SetAnimationIndex(int index) { animationIndex = index; }
	
	float GetProgressAnimationTime() { return progressAnimationTime; }	
	void SetProgressAnimationTime(float animTime) { progressAnimationTime = animTime; }

	bool GetIsAnimationPlay() { return isAnimPlay; }
	void SetIsAnimationPlay(bool value) { isAnimPlay = value; }

	float GetRoughness() { return roughness; }
	void SetRoughness(float value) { roughness = value > 1.0f ? 1.0f : value; }

	float GetMatalic() { return metalic; }
	void SetMatalic(float value) { metalic = value > 1.0f ? 1.0f : value; }

	Color GetColor() { return color; }
	void SetColor(Color value) { color = value; }

    void CreateBoneInfo();

private:
    FBXData* fbxData{};                 // 참조할 FBX 데이터

	// 모델 인스턴스 데이터
	std::string directory{};		    // 로드한 파일이 위차한 폴더명
	std::vector<Bone> bones{};			// 로드된 모델의 본 모음 -> 계층 구조에 있는 오브젝트들

	// 해당 모델의 상수 버퍼 내용
	BonePoseBuffer bonePoses{};

    // animation info
    int animationIndex = 0;             // 현재 실행 중인 애니메이션 인덱스
    float progressAnimationTime = 0.0f; // 현재 애니메이션 시간
    bool isAnimPlay = true;   

	float roughness = 0.0f;
	float metalic = 0.0f;
	Color color{};

    // == ID3D11Bufers ==
    ComPtr<ID3D11Buffer> bonePoseCB;
    ComPtr<ID3D11Buffer> boneOffsetCB;
    ComPtr<ID3D11Buffer> transformCB;
    ComPtr<ID3D11Buffer> materialCB;
};