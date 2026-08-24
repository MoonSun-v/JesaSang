#include "InteractionSensor.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"
#include "Object/GameObject.h"
#include "Components/FBXRenderer.h"

#include "../../Object/SearchObject.h"
#include "../../Object/HideObject.h"
#include "../../Object/TrapObject.h"
#include "../../JesaSang/JesaSangManager.h"
#include "../../Altar/AltarManager.h"
#include "../../CookingZone/CookingZone.h"
#include "../PlayerController.h"
#include "../DialogueUI/DialogueUIController.h"

#include "EngineSystem/PhysicsSystem.h"
#include <Util/PhysXUtils.h>

REGISTER_COMPONENT(InteractionSensor)

RTTR_REGISTRATION
{

    rttr::registration::class_<InteractionSensor>("InteractionSensor")
    .constructor<>()
    (rttr::policy::ctor::as_std_shared_ptr);
}

void InteractionSensor::OnStart()
{
   
}

void InteractionSensor::OnUpdate(float delta)
{
    // transform->physics udpate
    auto ob = GetOwner();
    auto tr = ob->GetTransform();
    if (!tr->GetParent()) return;

    Vector3 upatePos = tr->GetParent()->GetOwner()->GetTransform()->GetWorldPosition() + Vector3(0, 30, 0);
    tr->SetPosition(upatePos);
    GetOwner()->GetComponent<PhysicsComponent>()->SyncToPhysics();
}

nlohmann::json InteractionSensor::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void InteractionSensor::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void InteractionSensor::OnTriggerEnter(PhysicsComponent* other)
{
    GameObject* hitObject = other->GetOwner();
    string hitObjectName = hitObject->GetName();
    PlayerController* player = GetOwner()->GetParent()->GetOwner()->GetComponent<PlayerController>();

    // 수색 / 은신 중간에 막히는 오브젝트가 있다면 return 
    if (hitObjectName == "SearchObject" || hitObjectName == "HideObject")
    {
        // raycast
        Vector3 origin = GetOwner()->GetTransform()->GetLocalPosition() + Vector3{0, 100, 0};
        Vector3 target = hitObject->GetTransform()->GetLocalPosition();
        Vector3 dir = target - origin;
        float dist = dir.Length();
        dir.Normalize();

        vector<RaycastHit> hitBuffer;
        bool hit = PhysicsSystem::Instance().Raycast(
            origin,
            dir,
            dist,
            hitBuffer,
            CollisionLayer::Default,
            QueryTriggerInteraction::Ignore,
            true
        );

        if (hit)
        {
            // sort
            std::sort(hitBuffer.begin(), hitBuffer.end(),
                [](const RaycastHit& a, const RaycastHit& b)
                {
                    return a.distance < b.distance;
                });


            // blocked 여부 판단
            bool blocked = false;

            for (const auto& hitInfo : hitBuffer)
            {
                GameObject* rayHitObject = hitInfo.component ? hitInfo.component->GetOwner() : nullptr;
                if (!rayHitObject)
                    continue;

                // 플레이어, 인터랙션 센서, 인터랙션 존 무시
                string rayHitObjectName = rayHitObject->GetName();
                if (rayHitObjectName == "Player" || rayHitObjectName == "InteractionSensor" || rayHitObjectName == "InteractionZone")
                    continue;

                // 목표 오브젝트를 먼저 맞았으면 시야 확보된 것
                if (rayHitObject == hitObject)
                    break;
                   
                // 목표 전에 다른 뭔가를 먼저 맞았으면 막힌 것
                blocked = true;
                break;
            }

            if (blocked)
                return;
        }
    }
    
    // 수색 오브젝트 감지 on
    if (hitObjectName == "SearchObject")
    {
        if (player->IsInventoryFull()) return;

        auto* so = hitObject->GetComponent<SearchObject>();
        if (so)
            so->UISensorOnOff(true);
    }

    // 은신 오브젝트 감지 on
    if (hitObjectName == "HideObject")
    {
        auto* ho = hitObject->GetComponent<HideObject>();
        if (ho)
            ho->UISensorOnOff(true);
    }

    // 부엌 감지 on
    if (hitObjectName == "CookingZone")
    {
        if (!player->HasIngredient()) return;

        CookingZone::Instance()->UISensorOnOff(true);
    }

    // 제사상 감지 on
    if (hitObjectName == "JesaSang")
    {
        if (!player->HasFood()) return;

        JesaSangManager::Instance()->UISensorOnOff(true);
    }

    // 제단 감지 on
    if (hitObjectName == "Altar")
    {
        if (!AltarManager::Instance()->HasItem()) return;

        AltarManager::Instance()->UISensorOnOff(true);
    }

    // 함정 오브젝트 감지
    // 플레이어가 감지할 필요는 없지만, 최조 기믹 설명을 위해 여기만 추가
    if (other->GetOwner()->GetComponent<TrapObject>())
    {
        auto pc = GetOwner()->GetParent()->GetOwner()->GetComponent<PlayerController>();
        if (!pc->isExplainedTrapObject)
        {
            if (!pc->dialogueController)
            {
                std::cout << "[PlayerInteractionSensor] Dialogue Null? sibaaa" << std::endl;
                return;
            }
            pc->dialogueController->ShowInteractionHintAndPause(L"저 곳을 지나갈 때 조심하지 않으면 엄청 시끄러운 소리가 날 것 같아");
            pc->isExplainedTrapObject = true;
        }
    }
}

void InteractionSensor::OnTriggerExit(PhysicsComponent* other)
{
    GameObject* object = other->GetOwner();

    // 수색 오브젝트 감지 off
    if (object->GetName() == "SearchObject")
    {
        auto* so = object->GetComponent<SearchObject>();
        if (so)
            so->UISensorOnOff(false);
    }

    // 은신 오브젝트 감지 off
    if (object->GetName() == "HideObject")
    {
        auto* so = object->GetComponent<HideObject>();
        if (so)
            so->UISensorOnOff(false);
    }

    // 부엌 감지 off
    if (other->GetOwner()->GetName() == "CookingZone")
    {
        CookingZone::Instance()->UISensorOnOff(false);
    }

    // JesaSang 감지 off
    if (other->GetOwner()->GetName() == "JesaSang")
    {
        JesaSangManager::Instance()->UISensorOnOff(false);
    }

    // Altar 감지 off
    if (other->GetOwner()->GetName() == "Altar")
    {
        AltarManager::Instance()->UISensorOnOff(false);
    }
}
