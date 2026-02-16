#include "GhostInteractionZone.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"
#include "Object/GameObject.h"
#include "EngineSystem/SceneSystem.h"
#include "../../Woo/Player/PlayerController.h"
#include "AdultGhostController.h"
#include "../../Ho/Sound/EnemySoundSource.h"


REGISTER_COMPONENT(GhostInteractionZone)

RTTR_REGISTRATION
{

    rttr::registration::class_<GhostInteractionZone>("GhostInteractionZone")
    .constructor<>()
    (rttr::policy::ctor::as_std_shared_ptr);
}

nlohmann::json GhostInteractionZone::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void GhostInteractionZone::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}



void GhostInteractionZone::OnStart()
{

}

void GhostInteractionZone::OnFixedUpdate(float delta)
{
    // transform->physics udpate
    auto ob = GetOwner();
    auto tr = ob->GetTransform();
    Vector3 upatePos = tr->GetParent()->GetOwner()->GetTransform()->GetWorldPosition();
    tr->SetPosition(upatePos);
    GetOwner()->GetComponent<PhysicsComponent>()->SyncToPhysics();
}

void GhostInteractionZone::OnTriggerEnter(PhysicsComponent* other)
{
    if (other->GetOwner()->GetName() == "InteractZone")
    {
        auto* ghostObj = GetOwner()->GetTransform()->GetParent()->GetOwner();
        if (!ghostObj) return;

        auto* controller = ghostObj->GetComponent<AdultGhostController>();
        if (controller)
        {
            controller->OnAttackHit(); // FSM 한테 알림
        }

        //auto* player = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Player");
        //if (player)
        //{
        //    player->GetComponent<PlayerController>()->TakeAttack(); // AI에게 공격 당했을 때 
        //}
    }
}

void GhostInteractionZone::OnCCTTriggerEnter(CharacterControllerComponent* cct)
{
    if (cct->GetOwner()->GetName() == "Ghost_Baby")
    {
        // std::cout <<" [GhostInteractionZone] OnCCTTriggerEnter : Adult is Contact Ghost_Baby " << std::endl;
    }
}
