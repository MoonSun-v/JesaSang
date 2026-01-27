#pragma once
#include "../pch.h"

// [ RenderBlendType ]
// RenderComponent의 멤버로 소유됩니다.
// 하이브리드 렌더링 방식의 분기로 사용됩니다.
enum class RenderBlendType
{
    Opaque,
    Transparent
};


// [ Light Type ]
// Light Component의 멤버로 소유됩니다.
enum class LightType
{
    Directional = 0,
    Point = 1,
    Spot = 2
};


// [Decal Type]
// Decal Component의 멤버로 소유됩니다.
enum class DecalType
{
    TextureMap = 0,
    RingEffect = 1
};
