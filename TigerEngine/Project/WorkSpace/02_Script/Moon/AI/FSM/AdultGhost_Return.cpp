#include "AdultGhost_Return.h"

void AdultGhost_Return::Enter()
{
    cout << "[AdultGhost_Return] Enter Return State" << endl;

    adultGhost->ResetAgentForMove(3.0f); // Return 속도 

    adultGhost->animController->ChangeState("Idle");

    // 최근 웨이 포인트로 이동
    adultGhost->SetReturnToLastWaypoint();
}

void AdultGhost_Return::ChangeStateLogic()
{
    // 1. 시야에 플레이어가 들어오고, Hide 상태가 아니면 Chase
    auto* player = adultGhost->GetAITarget();
    if (player && !adultGhost->IsPlayerHidden() && adultGhost->CanDetectPlayer() /*adultGhost->IsSeeing(player)*/)
    {
        cout << "[AdultGhost_Return] Player detected -> Chase" << endl;
        adultGhost->ChangeState(AdultGhostState::Chase);
        return;
    }

    // 2. 도착 체크 : IsArrived()로 체크 (웨이포인트 좌표로 이동 완료 여부)
    if (adultGhost->agent->IsArrived())
    {
        adultGhost->ChangeState(AdultGhostState::Patrol);
    }
}

void AdultGhost_Return::Update(float deltaTime)
{
}

void AdultGhost_Return::FixedUpdate(float deltaTime)
{
}

void AdultGhost_Return::Exit()
{
    adultGhost->agent->ClearTarget();
}