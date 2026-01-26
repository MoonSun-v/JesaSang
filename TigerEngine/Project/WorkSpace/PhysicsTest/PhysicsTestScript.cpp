#include "PhysicsTestScript.h"
#include "Object/GameObject.h"
#include "System/InputSystem.h"
#include "Object/Component.h"
#include "Components/PhysicsComponent.h"
#include "Components/CharacterControllerComponent.h"


RTTR_REGISTRATION
{
    rttr::registration::class_<PhysicsTestScript>("PhysicsTestScript")
    .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}


void PhysicsTestScript::OnInitialize() 
{
    rigidComp = GetOwner()->GetComponent<PhysicsComponent>();
}

void PhysicsTestScript::OnStart() 
{
    if (rigidComp != nullptr)
    {
        rigidComp->CreateDynamicBox({ 20, 30, 20 }, 5);
        // rigidComp->CreateStaticBox({ 20, 30, 20 });
        // rigidComp->SetLayer(CollisionLayer::Player);
        rigidComp->SyncToPhysics();
    }
}

void PhysicsTestScript::OnUpdate(float delta)
{

}

void PhysicsTestScript::OnCollisionEnter(PhysicsComponent* other) 
{
    if (!other || !other->GetOwner()) return;

    auto name = this->GetOwner()->GetName();
    auto otherName = other->GetOwner()->GetName();

    cout << endl << "[Rigid]" << name << " - OnCollisionEnter : " << otherName << endl << endl;

    std::wstring wname(otherName.begin(), otherName.end());
    OutputDebugStringW((L"[Rigid] OnCollisionEnter : " + wname + L"\n").c_str());
}

//void PhysicsTestScript::OnCollisionStay(PhysicsComponent* other)
//{
//    if (!other || !other->GetOwner()) return;
//
//    auto name = other->GetOwner()->GetName();
//
//    cout << endl << "[Rigid] OnCollisionStay : " << name << endl << endl;
//
//    std::wstring wname(name.begin(), name.end());
//    OutputDebugStringW((L"[Rigid] OnCollisionStay : " + wname + L"\n").c_str());
//}

void PhysicsTestScript::OnCCTCollisionEnter(CharacterControllerComponent* cct)
{
    if (!cct || !cct->GetOwner()) return;

    auto name = cct->GetOwner()->GetName();

    cout << endl << "[Rigid] OnCCTCollisionEnter : " << name << endl << endl;

    std::wstring wname(name.begin(), name.end());
    OutputDebugStringW((L"[Rigid] OnCCTCollisionEnter : " + wname + L"\n").c_str());
}
