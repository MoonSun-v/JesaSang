#pragma once
#include "System/InputSystem.h"
#include "Entity/Component.h"
#include "Weapon.h"

/// <summary>
/// 클라이언트 컴포넌트 테스트용 클래스
/// </summary>
class Player1 : public Component, public InputProcesser
{
    RTTR_ENABLE(Component)
public:
    /// <summary>
    /// Component가 처음 실행될 때 실행됩니다.
    /// </summary>
    void OnInitialize() override;

    /// <summary>
    /// OnUpdate()를 실행하기 전 ***한 번*** 실행됩니다.
    /// </summary>
    void OnStart() override;

    /// <summary>
    /// GameEngine에 매 프레임마다 OnUpdate()내 에서 호출됩니다.
    /// </summary>
    void OnUpdate(float delta) override;

    void OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker,
        const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker) override;

private:
    Weapon* weapon{};
};

