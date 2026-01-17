#include "Player1.h"
#include "Entity/GameObject.h"
#include "System/InputSystem.h"
#include "Entity/Component.h"
#include "Components/FBXRenderer.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<Player1>("Player1")
    .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr)
    .property("r", &Player1::r)
    .property("g", &Player1::g)
    .property("b", &Player1::b);
}

void Player1::OnInitialize()
{
    InputSystem::Instance().Register(this);

    weapon = GetOwner()->AddComponent<Weapon>();
}

void Player1::OnStart()
{
}

void Player1::OnUpdate(float delta)
{
    auto comp = GetOwner()->GetComponent<FBXRenderer>();
    if (comp != nullptr)
    {
        comp->SetColor({ r, g, b, 1.0f });
    }

    r += delta;
    g += delta * 2.0f;
    b += delta * 3.0f;

    if (r > 1.0f) r = 0.f;
    if (g > 1.0f) g = 0.f;
    if (b > 1.0f) b = 0.f;
}

void Player1::OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker, const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker)
{
    if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::W))
    {
        GetOwner()->GetTransform()->position.y += 1.0f;
    }
    else if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::S))
    {
        GetOwner()->GetTransform()->position.y -= 1.0f;
    }
    if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::A))
    {
        GetOwner()->GetTransform()->position.x -= 1.0f;
    }
    else if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::D))
    {
        GetOwner()->GetTransform()->position.x += 1.0f;
    }
}
