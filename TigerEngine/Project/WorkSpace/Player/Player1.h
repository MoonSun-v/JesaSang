#pragma once
#include "System/InputSystem.h"
#include "Entity/ScriptComponent.h"
#include "Weapon.h"

/// <summary>
/// 클라이언트 컴포넌트 테스트용 클래스.
/// </summary>
class Player1 : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)
public:
    void OnInitialize() override;
    void OnStart() override;
    void OnUpdate(float delta) override;

    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;

private:
    Weapon* weapon{};
};

