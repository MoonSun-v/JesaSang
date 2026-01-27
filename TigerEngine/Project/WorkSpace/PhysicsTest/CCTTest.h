#pragma once
#include "Components/ScriptComponent.h"
#include "Components/CharacterControllerComponent.h"
#include "Components/AnimationController.h"

/// <summary>
/// 캐릭터 컨트롤러 컴포넌트 테스트용 클래스.
/// </summary>

class CCTTest : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    bool m_SpacePrev = false;

    CharacterControllerComponent* cctComp = nullptr;
    AnimationController* animController = nullptr;

    float elapsedTime = 0.0f;  // 시간 측정용
    bool hasRunStateChanged = false;

private:
    void CCTMoveExample(float dt); // 플레이어 이동 & 점프 예제 
    void RaycastExample(); // 레이캐스트 사용 & 디버그 예제 

public:
    void OnInitialize() override;
    void OnStart() override;
    void OnUpdate(float delta) override;
    void OnFixedUpdate(float dt) override;

    void OnCollisionEnter(PhysicsComponent* other) override;
};