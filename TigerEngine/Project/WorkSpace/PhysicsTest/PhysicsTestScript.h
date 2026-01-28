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

    // [ 충돌 감지 함수 종류 ]
    // OnCollision ~    : RigidComponent를 가진 객체를 감지 (Trigger X)
    // OnTrigger ~      : RigidComponent를 가진 객체를 감지 (Trigger O)
    // OnCCTCollision ~ : CharacterControllerComponent를 가진 객체를 감지 (Trigger X)
    // OnCCTTrigger ~   : CharacterControllerComponent를 가진 객체를 감지 (Trigger O)


    // [ rigid 감지하는 메소드 ] : RigidComponent를 가진 객체를 감지 한다. 
    void OnCollisionEnter(PhysicsComponent* other) override;
    //void OnCollisionStay(PhysicsComponent* other) override;
    //void OnCollisionExit(PhysicsComponent* other) override;

    //void OnTriggerEnter(PhysicsComponent* other) override;
    //void OnTriggerStay(PhysicsComponent* other) override;
    //void OnTriggerExit(PhysicsComponent* other) override;


    // [ CCT 감지하는 메소드 ] : CharacterControllerComponent를 가진 객체를 감지 한다. 
    void OnCCTCollisionEnter(CharacterControllerComponent* cct) override;
    //void OnCCTCollisionStay(CharacterControllerComponent* cct) override;
    //void OnCCTCollisionExit(CharacterControllerComponent* cct) override;

    //void OnCCTTriggerEnter(CharacterControllerComponent* cct) override;
    //void OnCCTTriggerStay(CharacterControllerComponent* cct) override;
    //void OnCCTTriggerExit(CharacterControllerComponent* cct) override;

};