#pragma once
#include "Components/ScriptComponent.h"

/// <summary>
/// 피직스 컴포넌트 테스트용 바닥 오브젝트 클래스.
/// </summary>

class GroundTestScript : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    PhysicsComponent* rigidComp;

public:
    void OnInitialize() override;
    void OnStart() override;
    void OnUpdate(float delta) override;

    void OnCCTCollisionEnter(CharacterControllerComponent* cct) override;
};