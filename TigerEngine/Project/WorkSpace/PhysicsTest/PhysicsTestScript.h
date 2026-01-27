#pragma once
#include "System/InputSystem.h"
#include "Components/ScriptComponent.h"

/// <summary>
/// 피직스 컴포넌트 테스트용 클래스.
/// </summary>

class PhysicsTestScript : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    PhysicsComponent* rigidComp;

public:
    void OnInitialize() override;
    void OnStart() override;
    void OnUpdate(float delta) override;

public:
    // [ rigid 감지하는 메소드 ]
    void OnCollisionEnter(PhysicsComponent* other) override;
    //void OnCollisionStay(PhysicsComponent* other) override;
    //void OnCollisionExit(PhysicsComponent* other) override;

    //void OnTriggerEnter(PhysicsComponent* other) override;
    //void OnTriggerStay(PhysicsComponent* other) override;
    //void OnTriggerExit(PhysicsComponent* other) override;


    // [ CCT 감지하는 메소드 ]
    //void OnCCTTriggerEnter(CharacterControllerComponent* cct) override;
    //void OnCCTTriggerStay(CharacterControllerComponent* cct) override;
    //void OnCCTTriggerExit(CharacterControllerComponent* cct) override;

    void OnCCTCollisionEnter(CharacterControllerComponent* cct) override;
    //void OnCCTCollisionStay(CharacterControllerComponent* cct) override;
    //void OnCCTCollisionExit(CharacterControllerComponent* cct) override;
};