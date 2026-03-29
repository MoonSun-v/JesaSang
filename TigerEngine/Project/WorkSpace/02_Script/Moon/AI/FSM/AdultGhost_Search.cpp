#include "AdultGhost_Search.h"
#include "EngineSystem/SceneSystem.h"


void AdultGhost_Search::Enter()
{
    cout << "[AdultGhost_Search] Enter Search State" << endl;

    if(adultGhost->IsPlayerHidden())
    {
        cout << "[AdultGhost_Search] Player hidden → Return\n";
        adultGhost->ChangeState(AdultGhostState::Return);
        return;
    }

    arrived = false;
    waitTimer = 0.0f;
    rotateTimer = 0.0f;

    adultGhost->ResetAgentForMove(3.5f);
    adultGhost->animController->ChangeState("Idle");

    phase = SearchPhase::WaitBeforeMove;

    // 1. Chase or Attack 에서 넘어온 경우 바로 이동
    if (adultGhost->searchReason == SearchReason::FromPatrol)
        phase = SearchPhase::WaitBeforeMove;
    else
        phase = SearchPhase::MoveToPoint;

    // 2. 마지막 플레이어 위치 있으면 사용 
    if (adultGhost->lastPlayerGrid.valid)
    {
        auto& p = adultGhost->lastPlayerGrid;
        adultGhost->agent->SetTarget(p.x, p.y);
        adultGhost->lastPlayerGrid.valid = false; // 1회성
    }

    // MoveToPoint인데 타겟이 없으면
    if (phase == SearchPhase::MoveToPoint && !adultGhost->agent->HasTarget())
    {
        phase = SearchPhase::RotateSearch;
        rotateTimer = 0.0f;
    }
}

void AdultGhost_Search::ChangeStateLogic()
{
    // 1. 플레이어 발견 (Search 성공)
    if (adultGhost->CanDetectPlayer())
    {
        cout << "[AdultGhost_Search] Player Search!! " << endl;
        adultGhost->ChangeState(AdultGhostState::Chase);
        return;
    }

    // 2. 회전 시간 종료 (Search 실패)
    if (phase == SearchPhase::RotateSearch && rotateTimer >= rotateTime)
    {
        cout << "[AdultGhost_Search] Search Fail.." << endl;
        adultGhost->ChangeState(AdultGhostState::Return);
    }
}

void AdultGhost_Search::Update(float deltaTime)
{
    if (adultGhost->IsPlayerHidden())
    {
        adultGhost->ChangeState(AdultGhostState::Return);
        return;
    }

    // 1. 어느 단계든 플레이어 감지
    if (adultGhost->CanDetectPlayer())
    {
        adultGhost->ChangeState(AdultGhostState::Chase);
        return;
    }

    // 2. WaitBeforeMove 단계
    if (phase == SearchPhase::WaitBeforeMove)
    {
        waitTimer += deltaTime;
        if (waitTimer >= waitTime)
        {
            if (adultGhost->agent->HasTarget())
                phase = SearchPhase::MoveToPoint;
            else
                phase = SearchPhase::RotateSearch; // 타겟 없으면 바로 회전
        }
    }

    // 3. MoveToPoint 단계
    else if (phase == SearchPhase::MoveToPoint)
    {
        if (adultGhost->agent->IsArrived())
        {
            phase = SearchPhase::RotateSearch;
            rotateTimer = 0.0f;
            adultGhost->agent->ClearTarget();
        }
    }

    // 4. RotateSearch (타이머 증가 담당)
    else if (phase == SearchPhase::RotateSearch)
    {
        rotateTimer += deltaTime;
    }
}

void AdultGhost_Search::FixedUpdate(float deltaTime)
{
    if (phase == SearchPhase::RotateSearch)
    {
        auto tr = adultGhost->GetOwner()->GetTransform();
        float newYaw = tr->GetYaw() + XMConvertToRadians(90.f) * deltaTime;
        tr->SetEuler(Vector3(0.0f, newYaw, 0.0f));
    }
}

void AdultGhost_Search::Exit()
{
    adultGhost->searchReason = SearchReason::None;
    if (adultGhost->agent)
        adultGhost->agent->ClearTarget();
}