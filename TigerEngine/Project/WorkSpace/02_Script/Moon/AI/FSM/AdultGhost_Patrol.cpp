#include "AdultGhost_Patrol.h"

#include "Components/VisionComponent.h"
#include "EngineSystem/SceneSystem.h"

#include "../../../Woo/Object/HideObject.h"


void AdultGhost_Patrol::Enter()
{
    cout << "[AdultGhost_Patrol] Enter Patrol State" << endl;

    agent = adultGhost->agent;

    adultGhost->ResetAgentForMove(0.8f);
    adultGhost->animController->ChangeState("Idle");

    // 무조건 웨이포인트 설정 
    adultGhost->SetNextPatrolTarget();
}

void AdultGhost_Patrol::ChangeStateLogic()
{
    // 1. 시야 감지 : 플레이어 가 범위 내에 진입 
    if (adultGhost->IsSeeing(adultGhost->GetAITarget()))
    {
        std::cout << "[AdultGhost_Patrol] Ghost is Seeing PLAYER !" << std::endl;
        adultGhost->ChangeState(AdultGhostState::Chase);
        return;
    }

    // 2. 기척 or 함정 감지 : 시야 밖에서 감지 
    if (!adultGhost->IsPlayerHidden() && adultGhost->IsPlayerInSenseRange())
    {
        std::cout << "[AdultGhost_Patrol] PLAYER FOUND (Sense)!" << std::endl;
        auto grid = GridSystem::Instance().GetMainGrid();
        if (grid)
        {
            int px, py;
            auto playerObj = adultGhost->GetPlayer();
            if (playerObj && grid->WorldToGridFromCenter(playerObj->GetTransform()->GetLocalPosition(), px, py))
            {
                adultGhost->lastPlayerGrid = { px, py, true };
            }
        }
        adultGhost->searchReason = SearchReason::FromPatrol;
        adultGhost->ChangeState(AdultGhostState::Search);
        return;
    }

    // Hide Object가 시야 내에 있는지 (플레이어 은신 불가능 해짐)
    UpdateHideObjectVision();
}

void AdultGhost_Patrol::Update(float deltaTime)
{
}

void AdultGhost_Patrol::FixedUpdate(float deltaTime)
{
    if (!agent) return;

    if (agent->IsArrived())
    {
        adultGhost->SetNextPatrolTarget();
    }
}


void AdultGhost_Patrol::Exit()
{
    if (adultGhost->curSeeingHideObject)
    {
        if (auto* hide = adultGhost->curSeeingHideObject->GetComponent<HideObject>())
            hide->UnregisterAILook(adultGhost);
    }
    adultGhost->curSeeingHideObject = nullptr;
    adultGhost->hideLookRegistered = false;

    if (agent)
        agent->ClearTarget();
}


// --------------------------------------------------------

void AdultGhost_Patrol::UpdateHideObjectVision()
{
    GameObject* seen = nullptr;

    for (auto* obj : adultGhost->hideObjects)
    {
        if (!obj) continue;

        if (adultGhost->IsSeeing(obj))
        {
            seen = obj;
            break;
        }
    }

    // 새로 봄
    if (seen && adultGhost->curSeeingHideObject != seen)
    {
        // 이전 해제
        if (adultGhost->curSeeingHideObject)
        {
            if (auto* hide = adultGhost->curSeeingHideObject->GetComponent<HideObject>())
                hide->UnregisterAILook(adultGhost);
        }

        adultGhost->curSeeingHideObject = seen;
        adultGhost->hideLookRegistered = true;

        if (auto* hide = seen->GetComponent<HideObject>())
            hide->RegisterAILook(adultGhost);
    }
    // 아무것도 안 보게 됨
    else if (!seen && adultGhost->curSeeingHideObject)
    {
        if (auto* hide = adultGhost->curSeeingHideObject->GetComponent<HideObject>())
            hide->UnregisterAILook(adultGhost);

        adultGhost->curSeeingHideObject = nullptr;
        adultGhost->hideLookRegistered = false;
    }
}