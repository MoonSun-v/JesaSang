#include "Components/PhysicsComponent.h"
#include "Object/GameObject.h"
#include "Components/CharacterControllerComponent.h"

#include "GroundTestScript.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<GroundTestScript>("GroundTestScript")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

void GroundTestScript::OnInitialize() 
{
    rigidComp = GetOwner()->GetComponent<PhysicsComponent>();
}

void GroundTestScript::OnStart()
{
    if (rigidComp != nullptr)
    {
        rigidComp->CreateStaticBox({ 600, 70, 600 });
        // rigidComp->CreateTriggerBox({ 600, 70, 600 });
        // rigidComp->SetLayer(CollisionLayer::IgnoreTest); // 충돌 레이어 테스트 
        rigidComp->SyncToPhysics();
    }
}

void GroundTestScript::OnUpdate(float delta)
{
    if (!rigidComp)
    {
        OutputDebugStringW(L"[GroundTestScript] OnUpdate의 rigidComp가 null입니다. \n");
        return;
    }
}

void GroundTestScript::OnCCTCollisionEnter(CharacterControllerComponent* cct)
{
    if (!cct || !cct->GetOwner()) return;

    auto name = this->GetOwner()->GetName();
    auto otherName = cct->GetOwner()->GetName();

    cout << endl << "[Rigid]" << name << " - OnCollisionEnter : " << otherName << endl << endl;

    std::wstring wName(name.begin(), name.end());
    std::wstring wOtherName(otherName.begin(), otherName.end());
    OutputDebugStringW((L"[Rigid] "+ wName + L" - OnCCTCollisionEnter : " + wOtherName + L"\n").c_str());
}
