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
    // Search Object
    if (other->GetOwner()->GetName() == "SearchObject")
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

    // Hide Object
    if (other->GetOwner()->GetName() == "HideObject")
    {
        auto hideOB = other->GetOwner()->GetComponent<HideObject>();
        if (hideOB)
        {
            player->SetCurHideObject(hideOB);
            hideOB->UIInteractionOnOff(true);
            cout << "[InteractionZone] HideObject In Interaction Zone" << endl;
        }
    }

    // CookingZone (MiniGame)
    if (other->GetOwner()->GetName() == "CookingZone")
    {
        if (!player->HasIngredient()) return;

        player->isPossibleCooking = true;
        CookingZone::Instance()->UIInteractionOnOff(true);
        cout << "[InteractionZone] CookingZone In Interaction Zone" << endl;
    }

    // JesaSang
    if (other->GetOwner()->GetName() == "JesaSang")
    {
        if (!player->HasFood()) return;

        player->isPossiblePutFood = true;
        JesaSangManager::Instance()->UIInteractionOnOff(true);
        cout << "[InteractionZone] JesaSang In Interaction Zone" << endl;
    }

    // Altar
    if (other->GetOwner()->GetName() == "Altar")
    {
        if (!AltarManager::Instance()->HasItem()) return;

        player->isPossibleGetFood = true;
        AltarManager::Instance()->UIInteractionOnOff(true);
        cout << "[InteractionZone] Altar In Interaction Zone" << endl;
    }
}

void InteractionZone::OnTriggerExit(PhysicsComponent* other)
{
    // Search Object
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

    // Hide Object
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

    // Cooking Zone (MiniGame)
    if (other->GetOwner()->GetName() == "CookingZone")
    {
        player->isPossibleCooking = false;
        CookingZone::Instance()->UIInteractionOnOff(false);
        cout << "[InteractionZone] CookingZone Out Interaction Zone" << endl;
    }

    // JesaSang
    if (other->GetOwner()->GetName() == "JesaSang")
    {
        player->isPossiblePutFood = false;
        JesaSangManager::Instance()->UIInteractionOnOff(false);
        cout << "[InteractionZone] JesaSang Out Interaction Zone" << endl;
    }

    // Altar
    if (other->GetOwner()->GetName() == "Altar")
    {
        player->isPossibleGetFood = false;
        AltarManager::Instance()->UIInteractionOnOff(false);
        cout << "[InteractionZone] Altar Out Interaction Zone" << endl;
    }
}
