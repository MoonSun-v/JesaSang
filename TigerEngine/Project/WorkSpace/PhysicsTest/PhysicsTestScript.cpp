#include "PhysicsTestScript.h"
#include "Object/GameObject.h"
#include "System/InputSystem.h"
#include "Object/Component.h"
#include "Components/PhysicsComponent.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<PhysicsTestScript>("PhysicsTestScript")
    .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}


void PhysicsTestScript::OnInitialize() // editor
{
}

void PhysicsTestScript::OnStart() // ?? 아직 사용하지X 
{

}

void PhysicsTestScript::OnUpdate(float delta)
{
    // OnStart() 대용 
    if (!isApply)
    {
        auto rigid = GetOwner()->GetComponent<PhysicsComponent>();
        if (rigid != nullptr)
        {
            rigid->CreateDynamicBox({ 20, 30, 20 }, 5);
            // rigid->SetLayer(CollisionLayer::Player);
            rigid->SyncToPhysics();
        }

        isApply = true;
    }
}