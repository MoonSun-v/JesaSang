#include "FreeCamera.h"
#include "../Object/GameObject.h"
#include "System/InputSystem.h"
#include "System/TimeSystem.h"

void FreeCamera::OnInitialize()
{
}

void FreeCamera::OnStart()
{
}

void FreeCamera::OnUpdate(float delta)
{
    auto trans = GetOwner()->GetTransform();

    // 속도 추가
    if (Input::GetKeyDown(DirectX::Keyboard::Keys::D1))
    {
        moveSpeed = 20;
    }
    if (Input::GetKeyDown(DirectX::Keyboard::Keys::D2))
    {
        moveSpeed = 100;
    }
    if (Input::GetKeyDown(DirectX::Keyboard::Keys::D3))
    {
        moveSpeed = 200;
    }

    // wasd
    Vector3 forward = GetForward();
    Vector3 right = GetRight();

    if (Input::GetKey(DirectX::Keyboard::Keys::W))
        SetInputVec(forward);
    else if (Input::GetKey(DirectX::Keyboard::Keys::S))
        SetInputVec(-forward);
    if (Input::GetKey(DirectX::Keyboard::Keys::A))
        SetInputVec(-right);
    else if (Input::GetKey(DirectX::Keyboard::Keys::D))
        SetInputVec(right);

    // qe
    if (Input::GetKey(DirectX::Keyboard::Keys::Q))
    {
        Matrix world = owner->GetTransform()->GetWorldTransform();
        SetInputVec(world.Up());
    }
    else if (Input::GetKey(DirectX::Keyboard::Keys::E))
    {
        Matrix world = owner->GetTransform()->GetWorldTransform();
        SetInputVec(-world.Up());
    }

    InputSystem::Instance().SetMouseMode(Input::GetMouseButton(1) ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);
    if (Input::GetMouseMode() == Mouse::MODE_RELATIVE)
    {
        float dx = float(Input::GetMouseX()) * rotSpeed;
        float dy = float(Input::GetMouseY()) * rotSpeed;
        AddPitch(dy);
        AddYaw(dx);
    }

    auto& transform = *owner->GetTransform();
    auto& position = transform.GetPosition();
    auto& rotation = transform.GetEuler();
    if (inputVec.Length() > 0.0f)
    {
        transform.Translate(inputVec * moveSpeed * delta);
        inputVec = Vector3::Zero;
    }
}

void FreeCamera::OnDestory()
{
}

void FreeCamera::AddPitch(float valueRad)
{
    auto rot = owner->GetTransform()->GetEuler();
    rot.x += valueRad;
    if (rot.x > XM_PIDIV2)  rot.x = XM_PIDIV2;  
    if (rot.x < -XM_PIDIV2) rot.x = -XM_PIDIV2;
    owner->GetTransform()->SetEuler(rot);
}

void FreeCamera::AddYaw(float valueRad)
{
    auto rot = owner->GetTransform()->GetEuler();
    rot.y += valueRad;
    if (rot.y > XM_PI)  rot.y -= XM_2PI;
    if (rot.y < -XM_PI) rot.y += XM_2PI;
    owner->GetTransform()->SetEuler(rot);
}

void FreeCamera::SetInputVec(const Vector3& inputVec)
{
    this->inputVec += inputVec;
    this->inputVec.Normalize();
}

Vector3 FreeCamera::GetForward()
{
    Matrix world = owner->GetTransform()->GetWorldTransform();
    return -world.Forward();
}

Vector3 FreeCamera::GetRight()
{
    Matrix world = owner->GetTransform()->GetWorldTransform();
    return world.Right();
}