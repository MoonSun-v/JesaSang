#include "Player1.h"
#include "Entity/GameObject.h"
#include "System/InputSystem.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<Player1>("Player1")
    .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);    
}

void Player1::OnInitialize()
{
    InputSystem::Instance().Register(this);

    weapon = owner->AddComponent<Weapon>();
}

void Player1::OnStart()
{
}

void Player1::OnUpdate(float delta)
{
}

void Player1::OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker, const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker)
{
    if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::W))
    {
        owner->GetTransform()->position.y += 1.0f;
    }
    else if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::S))
    {
        owner->GetTransform()->position.y -= 1.0f;
    }
    if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::A))
    {
        owner->GetTransform()->position.x -= 1.0f;
    }
    else if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::D))
    {
        owner->GetTransform()->position.x += 1.0f;
    }
}
