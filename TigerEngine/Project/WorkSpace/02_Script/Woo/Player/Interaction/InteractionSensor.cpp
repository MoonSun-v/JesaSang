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
    GameObject* object = other->GetOwner();
    if (!object) return;

    // 수색 오브젝트 감지 on
    if (object->GetName() == "SearchObject")
    {
        auto* so = object->GetComponent<SearchObject>();
        if (so)
            so->UISensorOnOff(true);
    }

    // 은신 오브젝트 감지 on
    if (object->GetName() == "HideObject")
    {
        auto* so = object->GetComponent<HideObject>();
        if (so)
            so->UISensorOnOff(true);
    }

    // 부엌 감지 on
    if (other->GetOwner()->GetName() == "CookingZone")
    {
        CookingZone::Instance()->UISensorOnOff(true);
    }

    // 제사상 감지 on
    if (other->GetOwner()->GetName() == "JesaSang")
    {
        JesaSangManager::Instance()->UISensorOnOff(true);
    }

    // 제단 감지 on
    if (other->GetOwner()->GetName() == "Altar")
    {
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
