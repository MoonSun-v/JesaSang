#pragma once
#include "../Object/Component.h"

/// <summary>
/// 사용자 정의 컴포넌트가 상속 받은 컴포넌트 클래스
/// 단순히 Engine부에서의 컴포넌트와 사용자가 만든 컴포넌트 분리용 클래스
/// </summary>
/// 
class ScriptComponent : public Component
{
    RTTR_ENABLE(Component)
public:
    ScriptComponent() = default;
    ~ScriptComponent() = default;

    bool IsPlayModeOnly() override { return true; }
};