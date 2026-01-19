#pragma once
#include "../Object/Component.h"

/// <summary>
/// free camera에 붙일 컴포넌트
/// </summary>
class FreeCamera : public Component
{
public:
    virtual void OnInitialize();

    virtual void OnStart();

    virtual void OnUpdate(float delta);

    virtual void OnDestory();

private:
    void AddPitch(float value);
    void AddYaw(float value);
    void SetInputVec(const Vector3& inputVec);
    Vector3 GetForward();
    Vector3 GetRight();

    Vector3 inputVec{};
    float rotSpeed = 0.004f;
    float moveSpeed = 100.0f;
};