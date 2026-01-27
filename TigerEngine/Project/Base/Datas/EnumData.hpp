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
