#include "BabyGhost_Patrol.h"

#include "Components/VisionComponent.h"
#include "EngineSystem/SceneSystem.h"

#include "../../../Woo/Object/HideObject.h"


void BabyGhost_Patrol::Enter()
{
    cout << "[BabyGhost_Patrol] Enter Patrol State" << endl;

    agent = babyGhost->agent;

    agent->externalControl = true;
    agent->patrolSpeed = 0.8f;  // Patrol 속도 
    agent->SetWaitTime(3.0f);   // 목표 지점에서 대기 시간 

    babyGhost->animController->ChangeState("Idle");

    //// 아직 이동 중이 아니라면 랜덤 목표 설정
    //if (!agent->hasTarget && !agent->isWaiting)
    //    agent->PickRandomTarget();

    if (!agent->hasTarget)
    {
        babyGhost->SetNextPatrolTarget();
    }

}

void BabyGhost_Patrol::ChangeStateLogic()
{
    // 1. 시야 감지 : 플레이어 가 범위 내에 진입 
    if (babyGhost->IsSeeing(babyGhost->GetAITarget()))
    {
        std::cout << "[BabyGhost_Patrol] Ghost is Seeing PLAYER !" << std::endl;
        babyGhost->ChangeState(BabyGhostState::Cry);
        return;
    }

    // 2. 기척 or 함정 감지 : 시야 밖에서 감지 
    if (babyGhost->IsPlayerInSenseRange())
    {
        std::cout << "[BabyGhost_Patrol] PLAYER FOUND (Sense)!" << std::endl;
        auto grid = GridSystem::Instance().GetMainGrid();
        if (grid)
        {
            int px, py;
            auto playerObj = babyGhost->GetPlayer();
            if (playerObj && grid->WorldToGridFromCenter(playerObj->GetTransform()->GetLocalPosition(), px, py))
            {
                babyGhost->lastPlayerGrid = { px, py, true };
            }
        }
        babyGhost->searchReason = SearchReason_Baby::FromPatrol;
        babyGhost->ChangeState(BabyGhostState::Search);
        return;
    }

    // Hide Object가 시야 내에 있는지 (플레이어 은신 불가능 해짐)
    UpdateHideObjectVision();
}

void BabyGhost_Patrol::Update(float deltaTime)
{
}

void BabyGhost_Patrol::FixedUpdate(float deltaTime)
{
    if (!agent) return;

    bool reached = babyGhost->MoveToTarget(deltaTime);

    if (reached)
    {
        agent->isWaiting = true;
        agent->waitTimer = agent->waitDuration;
        agent->hasTarget = false;
    }

    if (agent->isWaiting)
    {
        agent->waitTimer -= deltaTime;

        if (agent->waitTimer <= 0.0f)
        {
            agent->isWaiting = false;
            babyGhost->SetNextPatrolTarget();
        }
    }
}

void BabyGhost_Patrol::Exit()
{
    if (babyGhost->curSeeingHideObject)
    {
        if (auto* hide = babyGhost->curSeeingHideObject->GetComponent<HideObject>())
            hide->UnregisterAILook(babyGhost);
    }
    babyGhost->curSeeingHideObject = nullptr;
    babyGhost->hideLookRegistered = false;

    agent->hasTarget = false;
    agent->path.clear();
    agent->isWaiting = false;
}


// --------------------------------------------------------

void BabyGhost_Patrol::UpdateHideObjectVision()
{
    GameObject* seen = nullptr;

    for (auto* obj : babyGhost->hideObjects)
    {
        if (!obj) continue;

        if (babyGhost->IsSeeing(obj))
        {
            seen = obj;
            break;
        }
    }

    // 새로 봄
    if (seen && babyGhost->curSeeingHideObject != seen)
    {
        // 이전 해제
        if (babyGhost->curSeeingHideObject)
        {
            if (auto* hide = babyGhost->curSeeingHideObject->GetComponent<HideObject>())
                hide->UnregisterAILook(babyGhost);
        }

        babyGhost->curSeeingHideObject = seen;
        babyGhost->hideLookRegistered = true;

        if (auto* hide = seen->GetComponent<HideObject>())
            hide->RegisterAILook(babyGhost);
    }
    // 아무것도 안 보게 됨
    else if (!seen && babyGhost->curSeeingHideObject)
    {
        if (auto* hide = babyGhost->curSeeingHideObject->GetComponent<HideObject>())
            hide->UnregisterAILook(babyGhost);

        babyGhost->curSeeingHideObject = nullptr;
        babyGhost->hideLookRegistered = false;
    }
}