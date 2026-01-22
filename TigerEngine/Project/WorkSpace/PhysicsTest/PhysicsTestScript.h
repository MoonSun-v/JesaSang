#pragma once
#include "System/InputSystem.h"
#include "Components/ScriptComponent.h"

/// <summary>
/// 피직스 컴포넌트 테스트용 클래스.
/// </summary>

// - inspcter에서 PhysicsTestScript + PhysicsComponent 추가
// - EngineApp에 ComponentFactory::Instance().Register<PhysicsTestScript>("PhysicsTestScript");

class PhysicsTestScript : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

public:
    void OnInitialize() override;
    void OnStart() override;
    void OnUpdate(float delta) override;

    bool isApply = false;

};