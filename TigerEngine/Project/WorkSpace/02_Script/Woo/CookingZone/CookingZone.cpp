#include "CookingZone.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"
#include "EngineSystem/SceneSystem.h"
#include "Components/Decal.h"
#include "Components/Transform.h"
#include "Object/GameObject.h"
#include "Components/UI/Image.h"
#include "System/TimeSystem.h"
#include "../../Moon/AI/AdultGhostController.h"

REGISTER_COMPONENT(CookingZone)

RTTR_REGISTRATION
{
    rttr::registration::class_<CookingZone>("CookingZone")
    .constructor<>()
    (rttr::policy::ctor::as_std_shared_ptr);
}

void CookingZone::OnInitialize()
{
    // 중복 생성 방지
    if (s_instance != nullptr && s_instance != this)
    {
        assert(false && "Duplicate GameManager instance detected.");
        return;
    }

    s_instance = this;
}

void CookingZone::OnStart()
{
    const auto& sceneSystem = SceneSystem::Instance().GetCurrentScene();

    // cooking zone ui find
    image_sensorOn = sceneSystem->GetGameObjectByName("Image_SensorOn_Cook")->GetComponent<Image>();
    image_interactionOn = sceneSystem->GetGameObjectByName("Image_InteractionOn_Cook")->GetComponent<Image>();
    image_interactionGauge = sceneSystem->GetGameObjectByName("Image_InteractionGauge_Cook")->GetComponent<Image>();

    if (!image_sensorOn || !image_interactionOn || !image_interactionGauge)
    {
        cout << "[CookingZone] Missing ui!" << endl;
        return;
    }

    // effect find
    ringEffect = this->GetOwner()->GetChildByName("RingEffect")->GetOwner()->GetComponent<Decal>();
    if (!ringEffect)
        cout << "[TrapObject] Missing Decal Component!" << endl;
}

void CookingZone::OnDestory()
{
    if (s_instance == this) s_instance = nullptr;
}

nlohmann::json CookingZone::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void CookingZone::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void CookingZone::UISensorOnOff(bool flag)
{
    if (!image_sensorOn) return;
    image_sensorOn->SetActive(flag);
}

void CookingZone::UIInteractionOnOff(bool flag)
{
    if (!image_interactionOn) return;
    image_interactionOn->SetActive(flag);
    image_interactionGauge->SetActive(flag);
}

void CookingZone::UIGaugeUpate(float progress)
{
    if (!image_interactionGauge) return;
    image_interactionGauge->SetFillAmount(progress);
}

// 플레이어 미니게임 실패시 호출
void CookingZone::StartTriggerWave()
{
    // 링 파동 이펙트
    auto curTime = GameTimer::Instance().TotalTime();
    ringEffect->StartRingEffect(1,curTime);

    // AI
    NotifyAIInRange();
}

void CookingZone::NotifyAIInRange()
{
    // 어른 유령 호출  // TODO :: 1마리만!!
    auto adultGhosts = SceneSystem::Instance().GetCurrentScene()->GetGameObjectsByName("Ghost_Adult");
    if (!adultGhosts.empty())
    {
        for (auto ag : adultGhosts)
        {
            Vector3 originPos = this->GetOwner()->GetTransform()->GetWorldPosition();
            Vector3 targetPos = ag->GetTransform()->GetWorldPosition();

            float dist = Vector3::Distance(originPos, targetPos);

            // 파동 범위 안에 AI가 있다면 호출
            if (dist <= radius)
            {
                ag->GetComponent<AdultGhostController>()->OnPlayerNoise(originPos);
            }
        }
    }
}
