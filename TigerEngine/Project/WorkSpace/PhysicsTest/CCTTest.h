#pragma once
#include "Components/ScriptComponent.h"
#include "Components/CharacterControllerComponent.h"

/// <summary>
/// 캐릭터 컨트롤러 컴포넌트 테스트용 클래스.
/// </summary>

class CCTTest : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    bool isApply = false;
    bool m_SpacePrev = false;

    CharacterControllerComponent* cctComp = nullptr;

public:
    void OnInitialize() override;
    void OnStart() override;
    void OnUpdate(float delta) override;
    void OnFixedUpdate(float dt) override;
};