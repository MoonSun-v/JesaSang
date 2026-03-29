#include "BabyGhost_Search.h"
#include "EngineSystem/SceneSystem.h"


void BabyGhost_Search::Enter()
{
    cout << "[BabyGhost_Search] Enter Search State" << endl;

    arrived = false;
    waitTimer = 0.0f;
    rotateTimer = 0.0f;

    babyGhost->ResetAgentForMove(4.0f);
    babyGhost->animController->ChangeState("Idle");

    // Patrol에서 왔으면 3초 대기, 그 외엔 바로 이동
    if (babyGhost->searchReason == SearchReason_Baby::FromPatrol)
        phase = SearchPhase::WaitBeforeMove;
    else
        phase = SearchPhase::MoveToPoint;

    // 마지막 감지 위치로 이동
    if (babyGhost->lastPlayerGrid.valid)
    {
        auto& p = babyGhost->lastPlayerGrid;
        babyGhost->agent->SetTarget(p.x, p.y);
        babyGhost->lastPlayerGrid.valid = false;
    }

    // 바로 이동할 타겟이 없으면 회전 탐색으로
    if (phase == SearchPhase::MoveToPoint && !babyGhost->agent->HasTarget())
    {
        phase = SearchPhase::RotateSearch;
        rotateTimer = 0.0f;
    }
}

void BabyGhost_Search::ChangeStateLogic()
{
    // 1. 플레이어 발견 (Search 성공)
    if (babyGhost->IsSeeing(babyGhost->GetAITarget()))
    {
        cout << "[BabyGhost_Search] Search Clear!! " << endl;
        babyGhost->ChangeState(BabyGhostState::Cry);
        return;
    }
    // 2. 회전 시간 종료 (Search 실패)
    if (phase == SearchPhase::RotateSearch && rotateTimer >= rotateTime)
    {
        cout << "[BabyGhost_Search] Search Fail -> Return" << endl;
        babyGhost->ChangeState(BabyGhostState::Return); 
        return;
    }
}

void BabyGhost_Search::Update(float deltaTime)
{
    if (phase == SearchPhase::WaitBeforeMove)
    {
        waitTimer += deltaTime;
        if (waitTimer >= waitTime)
        {
            if (babyGhost->agent->HasTarget())
                phase = SearchPhase::MoveToPoint;
            else
                phase = SearchPhase::RotateSearch;
        }
    }
    else if (phase == SearchPhase::MoveToPoint)
    {
        if (babyGhost->agent->IsArrived())
        {
            phase = SearchPhase::RotateSearch;
            rotateTimer = 0.0f;
            babyGhost->agent->ClearTarget();
        }
    }
    else if (phase == SearchPhase::RotateSearch)
    {
        rotateTimer += deltaTime;
    }
}

void BabyGhost_Search::FixedUpdate(float deltaTime)
{
    if (phase == SearchPhase::RotateSearch)
    {
        auto tr = babyGhost->GetOwner()->GetTransform();
        float newYaw = tr->GetYaw() + XMConvertToRadians(90.f) * deltaTime;
        tr->SetEuler(Vector3(0.f, newYaw, 0.f));
    }
}

void BabyGhost_Search::Exit()
{
    babyGhost->searchReason = SearchReason_Baby::None; // 이유 초기화
    if (babyGhost->agent)
        babyGhost->agent->ClearTarget();
}