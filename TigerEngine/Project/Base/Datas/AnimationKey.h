#pragma once
#include "../pch.h"

using namespace DirectX::SimpleMath;

struct PositionKey
{
    float Time = 0.0f;
    Vector3 Value = Vector3::Zero;
};
struct RotationKey
{
    float Time = 0.0f;
    Quaternion Value = Quaternion::Identity;
};
struct ScaleKey
{
    float Time = 0.0f;
    Vector3 Value = Vector3::One;
};