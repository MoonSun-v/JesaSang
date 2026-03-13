#include "InteractionZone.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"
#include "EngineSystem/PhysicsSystem.h"

#include "Object/GameObject.h"
#include "../../Object/SearchObject.h"
#include "../../Object/HideObject.h"
#include "../../JesaSang/JesaSangManager.h"
#include "../../Altar/AltarManager.h"
#include "../../CookingZone/CookingZone.h"

#include <Util/PhysXUtils.h>


REGISTER_COMPONENT(InteractionZone)

RTTR_REGISTRATION
{

    rttr::registration::class_<InteractionZone>("InteractionZone")
    .constructor<>()
    (rttr::policy::ctor::as_std_shared_ptr);
}

void InteractionZone::OnStart()
{
    player = this->GetOwner()->GetTransform()->GetParent()->GetOwner()->GetComponent<PlayerController>();
    if (!player)
        cout << "[InteractionSensor] player component missing!" << endl;
}

void InteractionZone::OnUpdate(float delta)
{
    
}

void InteractionZone::OnFixedUpdate(float delta)
{
    // transform->physics udpate
    auto ob = GetOwner();
    auto tr = ob->GetTransform();
    Vector3 upatePos = tr->GetParent()->GetOwner()->GetTransform()->GetWorldPosition() + Vector3(0,70,0);
    tr->SetPosition(upatePos);
    GetOwner()->GetComponent<PhysicsComponent>()->SyncToPhysics();
}

nlohmann::json InteractionZone::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void InteractionZone::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void InteractionZone::OnTriggerEnter(PhysicsComponent* other)
{
    GameObject* hitObject = other->GetOwner();
    string hitObjectName = hitObject->GetName();

    // 수색 / 은신 중간에 막히는 오브젝트가 있다면 return 
    if (hitObjectName == "SearchObject" || hitObjectName == "HideObject")
    {
        // raycast
        Vector3 origin = GetOwner()->GetParent()->GetLocalPosition() + Vector3{ 0, 100, 0 };
        Vector3 target = hitObject->GetTransform()->GetLocalPosition();
        Vector3 dir = target - origin;
        float dist = dir.Length();
        dir.Normalize();

        vector<RaycastHit> hitBuffer;
        bool hit = PhysicsSystem::Instance().Raycast(
            ToPx(origin),
            ToPx(dir),
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

    // Search Object 인터랙션 가능 on
    if (hitObjectName == "SearchObject")
    {
        auto searchOB = other->GetOwner()->GetComponent<SearchObject>();
        if(searchOB && !searchOB->isSearched)
        {
            if (player->IsInventoryFull()) return;

            player->SetCurSearchObject(searchOB);
            searchOB->UIInteractionOnOff(true);
            cout << "[InteractionZone] SearchObject In Interaction Zone" << endl;
        }
    }

    // Hide Object 인터랙션 가능 on
    if (hitObjectName == "HideObject")
    {
        auto hideOB = other->GetOwner()->GetComponent<HideObject>();
        if (hideOB)
        {
            player->SetCurHideObject(hideOB);
            hideOB->UIInteractionOnOff(true);
            cout << "[InteractionZone] HideObject In Interaction Zone" << endl;
        }
    }

    // CookingZone (MiniGame) 인터랙션 가능 on
    if (hitObjectName == "CookingZone")
    {
        if (!player->HasIngredient()) return;

        player->isPossibleCooking = true;
        CookingZone::Instance()->UIInteractionOnOff(true);
        cout << "[InteractionZone] CookingZone In Interaction Zone" << endl;
    }

    // JesaSang 인터랙션 가능 on
    if (hitObjectName == "JesaSang")
    {
        if (!player->HasFood()) return;

        player->isPossiblePutFood = true;
        JesaSangManager::Instance()->UIInteractionOnOff(true);
        cout << "[InteractionZone] JesaSang In Interaction Zone" << endl;
    }

    // Altar 인터랙션 가능 on
    if (hitObjectName == "Altar")
    {
        if (!AltarManager::Instance()->HasItem()) return;

        player->isPossibleGetFood = true;
        AltarManager::Instance()->UIInteractionOnOff(true);
        cout << "[InteractionZone] Altar In Interaction Zone" << endl;
    }
}

void InteractionZone::OnTriggerExit(PhysicsComponent* other)
{
    // Search Object 인터랙션 가능 off
    if (other->GetOwner()->GetName() == "SearchObject")
    {
        auto searchOB = other->GetOwner()->GetComponent<SearchObject>();
        if (searchOB && !searchOB->isSearched)
        {
            player->SetCurSearchObject(nullptr);
            searchOB->UIInteractionOnOff(false);
            cout << "[InteractionZone] SearchObject Out Interaction Zone" << endl;
        }
    }

    // Hide Object 인터랙션 가능 off
    if (other->GetOwner()->GetName() == "HideObject")
    {
        auto hideOB = other->GetOwner()->GetComponent<HideObject>();
        if (hideOB)
        {
            player->SetCurHideObject(nullptr);
            hideOB->UIInteractionOnOff(false);
            cout << "[InteractionZone] HideObject Out Interaction Zone" << endl;
        }
    }

    // Cooking Zone (MiniGame) 인터랙션 가능 off
    if (other->GetOwner()->GetName() == "CookingZone")
    {
        player->isPossibleCooking = false;
        CookingZone::Instance()->UIInteractionOnOff(false);
        cout << "[InteractionZone] CookingZone Out Interaction Zone" << endl;
    }

    // JesaSang 인터랙션 가능 off
    if (other->GetOwner()->GetName() == "JesaSang")
    {
        player->isPossiblePutFood = false;
        JesaSangManager::Instance()->UIInteractionOnOff(false);
        cout << "[InteractionZone] JesaSang Out Interaction Zone" << endl;
    }

    // Altar 인터랙션 가능 off
    if (other->GetOwner()->GetName() == "Altar")
    {
        player->isPossibleGetFood = false;
        AltarManager::Instance()->UIInteractionOnOff(false);
        cout << "[InteractionZone] Altar Out Interaction Zone" << endl;
    }
}
