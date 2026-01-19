#pragma once
#include "pch.h"
#include "../Object/Component.h"
#include "../RenderQueue/RenderQueue.h"

enum class RenderType
{
    None = 0,
    StaticMesh = 1 << 1,
    DynamicMesh = 1 << 2,
    SkeltalMesh = 1 << 3
};

/// @brief 렌더링 처리하는 컴포넌트가 상속받는 컴포넌트
class RenderComponent : public Component
{        
    // RTTR_ENABLE(Component)
public:
    RenderType renderType;

    /// <summary>
    /// renderpass 에서 호출되는 함수
    /// </summary>
    /// <param name="context"></param>
    virtual void OnRender(RenderQueue& queue) = 0;
protected:
};