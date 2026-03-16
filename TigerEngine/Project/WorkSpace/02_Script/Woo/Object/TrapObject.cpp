#include "TrapObject.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"
#include "Object/GameObject.h"
#include "../Player/PlayerController.h"
#include "EngineSystem/SceneSystem.h"
#include "Components/Decal.h"
#include "Components/Transform.h"
#include "System/TimeSystem.h"
#include "../../Moon/AI/AdultGhostController.h"
#include "../../Moon/AI/BabyGhostController.h"
#include "../../Ho/Sound/SoundManager.h"


REGISTER_COMPONENT(TrapObject)

RTTR_REGISTRATION
{
    rttr::registration::class_<TrapObject>("TrapObject")
    .constructor<>()
    (rttr::policy::ctor::as_std_shared_ptr)
    .property("isTable", &TrapObject::isTable);
}

void TrapObject::OnStart()
{
    ringEffect = this->GetOwner()->GetChildByName("RingEffect")->GetOwner()->GetComponent<Decal>();
    if (!ringEffect)
        cout << "[TrapObject] Missing Decal Component!" << endl;
}

void TrapObject::OnUpdate(float delta)
{
    // wave 파동중일때 re wave cool time udapte
    if (!isWaveing)
        return;

    coolTimer += delta;
    if (coolTimer >= coolTime)
    {
        // 재발동 가능
        isPossibleWave = true;
        isWaveing = false;

        // clear
        coolTimer = 0.0f;
        curWaveRadius = 0.0f;
    }
}

nlohmann::json TrapObject::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void TrapObject::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void TrapObject::OnCCTTriggerEnter(CharacterControllerComponent* other)
{
    if (!isPossibleWave) return;        // 최초1회, re wave 가능할때

    GameObject* otherOwner = other->GetOwner();
    if (otherOwner->GetName() != "Player") return;

    // 플레이어 상태에 따른 파장 범위 설정
    auto pc = otherOwner->GetComponent<PlayerController>();
    if (!pc) return;

    auto state = pc->GetPlayerState();

    switch (state)
    {
    case PlayerState::Walk:
        curWaveRadius = walkWaveRadius;
        curWaveSpeed = walkWaveSpeed;
        if(isTable) curAiRange = walkAiRange_table;
        else curAiRange = walkAiRange_decal;
        break;

    case PlayerState::Run:
    case PlayerState::Hit:
        curWaveRadius = runWaveRadius;
        curWaveSpeed = runWaveSpeed;
        if (isTable) curAiRange = runAiRange_table;
        else curAiRange = runAiRange_decal;
        break;

    default:
        return; // 파장 발생 안 함
    }

    // 파장 웨이브 시작
    StartTriggerWave();     
}


void TrapObject::StartTriggerWave()
{
    // 링 파동 이펙트
    auto curTime = GameTimer::Instance().TotalTime();
    ringEffect->StartRingEffect(curWaveRadius, curTime, 8, curWaveSpeed);

    // AI
    NotifyAIInRange();

    // sound
    SoundManager::Instance()->PlaySFX(SFXType::PitfallObj_Floor_Sound);
    
    // re wave cool time start
    isWaveing = true;
    isPossibleWave = false;
    coolTimer = 0.0f;

    cout << "[TrapObject] : Start Trap Trigger Wave. Range : " << curWaveRadius << endl;
}

void TrapObject::NotifyAIInRange()
{
    /*
        파동 발생한 즉시 월드의 AI를 모두 찾아서 dist > waveLength보다 크다면 AI Called
        - Ghost_Adult <AdultGhostController>
        - Ghost_Baby
    */

    // 어른 유령 호출
    auto adultGhosts = SceneSystem::Instance().GetCurrentScene()->GetGameObjectsByName("Ghost_Adult");
    if (!adultGhosts.empty())
    {
        for (auto ag : adultGhosts)
        {
            Vector3 originPos = this->GetOwner()->GetTransform()->GetWorldPosition();
            Vector3 targetPos = ag->GetTransform()->GetWorldPosition();
            
            float dist = Vector3::Distance(originPos, targetPos);

            // 파동 범위 안에 AI가 있다면 호출
            if (dist <= curAiRange)
            {
                ag->GetComponent<AdultGhostController>()->OnPlayerNoise(originPos);
            }
        }
    }

    // 애기 유령 호출
    auto babyGhosts = SceneSystem::Instance().GetCurrentScene()->GetGameObjectsByName("Ghost_Baby");
    if (!babyGhosts.empty())
    {
        for (auto bg : babyGhosts)
        {
            Vector3 originPos = this->GetOwner()->GetTransform()->GetWorldPosition();
            Vector3 targetPos = bg->GetTransform()->GetWorldPosition();

            float dist = Vector3::Distance(originPos, targetPos);

            // 파동 범위 안에 AI가 있다면 호출
            if (dist <= curWaveRadius)
            {
                bg->GetComponent<BabyGhostController>()->OnPlayerNoise(originPos);
            }
        }
    }
}
