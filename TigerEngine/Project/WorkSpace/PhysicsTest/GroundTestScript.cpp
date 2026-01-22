#include "Components/PhysicsComponent.h"
#include "Object/GameObject.h"

#include "GroundTestScript.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<GroundTestScript>("PhysicsTestScript")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

void GroundTestScript::OnInitialize() // editor
{
}

void GroundTestScript::OnStart() // ?? 아직 사용하지X 
{

}

void GroundTestScript::OnUpdate(float delta)
{
    // OnStart() 대용 
    if (!isApply)
    {
        auto rigid = GetOwner()->GetComponent<PhysicsComponent>();
        if (rigid != nullptr)
        {
            rigid->CreateStaticBox({ 600, 70, 600 });
            rigid->SetLayer(CollisionLayer::IgnoreTest); // 충돌 레이어 테스트 
            rigid->SyncToPhysics();
        }

        isApply = true;
    }
}