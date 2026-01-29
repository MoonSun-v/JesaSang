#pragma once
#include "../../../Engine/Components/ScriptComponent.h"
#include "../../../Engine/Components/CharacterControllerComponent.h"

// 우정
// Player Move Test (Physics + CCT)
// 이거 만들어보고 FSM 넣을거

class PlayerMove_Test : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    CharacterControllerComponent* cctComp = nullptr;
    bool m_SpacePrev = false;

public:
    // component process
    void OnInitialize() override;
    void OnStart() override;
    void OnUpdate(float delta) override;
    void OnFixedUpdate(float dt) override;

    // collision event
    void OnCollisionEnter(PhysicsComponent* other) override;

public:
    void Move(float dt);
};

