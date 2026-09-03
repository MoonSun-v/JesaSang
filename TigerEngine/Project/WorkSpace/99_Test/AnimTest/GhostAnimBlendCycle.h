#pragma once
#include "Components/ScriptComponent.h"

class FBXData;
class AnimationController;

/// <summary>
/// [ 임시 촬영용 테스트 스크립트 ]
/// 아기 귀신의 "Idle" <-> "Cry" 애니메이션을 일정 시간마다 자동으로 전환하며
/// 계속 반복 재생한다. (04_MainGameScene의 BabyGhostController가 실제로 사용하는
/// 애니메이션만 동일하게 사용한다.)
///
/// UseBlending = false : 블렌딩 없이 즉시 포즈가 바뀌어 뚝뚝 끊기듯 어색하게 전환됨
/// UseBlending = true  : BlendDuration 만큼 자연스럽게 크로스페이드되며 전환됨
/// 두 인스턴스를 나란히 배치해서 TRS 블렌딩 유무 차이를 비교 촬영하는 용도.
/// </summary>
class GhostAnimBlendCycle : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

public:
    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

    // 각 상태를 유지하는 시간(초)
    float IdleHoldTime = 3.5f;
    float CryHoldTime = 3.5f;

    // true : BlendDuration 만큼 자연스럽게 전환 / false : 블렌딩 없이 즉시 전환(어색한 팝)
    bool UseBlending = false;
    float BlendDuration = 0.35f;

    void OnStart() override;
    void OnUpdate(float delta) override;

private:
    void LoadAnimations();
    void SwitchState();

    FBXData* fbxData = nullptr;
    AnimationController* animController = nullptr;

    bool isCrying = false;
    float timer = 0.0f;
};
