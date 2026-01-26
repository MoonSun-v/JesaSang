#pragma once
#include "pch.h"
#include "../Object/Component.h"
#include "../RenderQueue/RenderQueue.h"
#include "../Base/Datas/EnumData.hpp"

/// @brief 렌더링 처리하는 컴포넌트가 상속받는 컴포넌트
class RenderComponent : public Component
{        
    // RTTR_ENABLE(Component)
public:
    RenderBlendType renderBlendType = RenderBlendType::Opaque;

    /// <summary>
    /// renderpass 에서 호출되는 함수
    /// </summary>
    /// <param name="context"></param>
    virtual void OnRender(RenderQueue& queue) = 0;
protected:
};