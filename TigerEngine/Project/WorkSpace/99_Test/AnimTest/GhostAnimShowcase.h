#pragma once
#include "Components/ScriptComponent.h"

class FBXData;
class AnimationController;

/// <summary>
/// [ 임시 촬영용 테스트 스크립트 ]
/// AgentComponent / AdultGhostController 없이, 지정된 애니메이션 상태
/// ("Idle" / "Attack" / "AttackDelay")를 AI 로직 없이 바로 재생해서 보여준다.
/// 어른 귀신은 별도의 Walk(이동) 애니메이션이 없어서, 04_MainGameScene의
/// AdultGhostController가 실제로 사용하는 애니메이션만 그대로 사용한다.
/// 씬에 Ghost_Adult 모델을 배치하고 이 컴포넌트만 붙여서 각 인스턴스마다
/// PlayState를 다르게 지정하면, 각 상태의 포즈/애니메이션을 정지 화면이나
/// 영상으로 촬영하기 위한 용도로 쓸 수 있다.
/// </summary>
class GhostAnimShowcase : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

public:
    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

    // 인스펙터 / 씬 데이터로 지정 : "Idle" / "Attack" / "AttackDelay"
    std::string PlayState = "Idle";

    void OnStart() override;

private:
    void LoadAnimations();

    FBXData* fbxData = nullptr;
    AnimationController* animController = nullptr;
};
