#include "AdultGhost_Return.h"

void AdultGhost_Return::Enter()
{
    cout << "[AdultGhost_Return] Enter Return State" << endl;

    adultGhost->ResetAgentForMove(3.0f); // Return 속도 

    adultGhost->animController->ChangeState("Idle");

    // 최근 웨이 포인트로 이동
    adultGhost->SetReturnToLastWaypoint();

    //auto grid = GridSystem::Instance().GetMainGrid();
    //if (grid)
    //{
    //    int tx, ty;
    //    if (grid->WorldToGridFromCenter(adultGhost->initialPosition, tx, ty)) // 웨이 포인트 = AI가 처음 배치된 위치가 아니라 마지막에 이동중이던 웨이포인트로 변경
    //    {
    //        adultGhost->agent->SetTarget(tx, ty);
    //    }
    //}
}

void AdultGhost_Return::ChangeStateLogic()
{
    // 1. 시야에 플레이어가 들어오면 Chase
    auto* player = adultGhost->GetAITarget();
    if (player && adultGhost->IsSeeing(player))
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

    //// 2. 복귀 완료 (그리드 좌표 적용) : 현재 위치와 initialPosition 비교
    //auto grid = GridSystem::Instance().GetMainGrid();
    //if (!grid) return;

    //int cx, cy;
    //if (!grid->WorldToGridFromCenter(adultGhost->GetOwner()->GetTransform()->GetWorldPosition(), cx, cy))
    //    return;

    //// 웨이포인트 그리드 좌표
    //int wx, wy;
    //if (!grid->WorldToGridFromCenter(adultGhost->initialPosition, wx, wy))
    //    return;
    //if (cx == wx && cy == wy)
    //{
    //    cout << "[AdultGhost_Return] Reached waypoint -> Patrol" << endl;
    //    adultGhost->ChangeState(AdultGhostState::Patrol);
    //    return;
    //}


    //// 복귀 완료 : 현재 위치와 initialPosition 비교
    //auto tr = adultGhost->GetOwner()->GetTransform();
    //float distSqr = (tr->GetWorldPosition() - adultGhost->initialPosition).LengthSquared();
    //const float arrivalThreshold = 150.0f; // 거의 도착했으면
    //if (distSqr <= arrivalThreshold * arrivalThreshold)
    //{
    //    cout << "[AdultGhost_Return] Reached waypoint -> Patrol" << endl;
    //    adultGhost->ChangeState(AdultGhostState::Patrol);
    //    return;
    //}
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