#include "BabyGhost_Search.h"
#include "EngineSystem/SceneSystem.h"


void BabyGhost_Search::Enter()
{
    cout << "[BabyGhost_Search] Enter Search State" << endl;

    arrived = false;
    waitTimer = 0.0f;
    rotateTimer = 0.0f;

    babyGhost->ResetAgentForMove(3.5f);
    babyGhost->animController->ChangeState("Idle");

    phase = SearchPhase::WaitBeforeMove;

    // 마지막 플레이어 위치 있으면 사용 
    if (babyGhost->lastPlayerGrid.valid /*&&
        (babyGhost->searchReason == SearchReason::FromChase || babyGhost->searchReason == SearchReason::FromAttack)*/)
    {
        auto& p = babyGhost->lastPlayerGrid;
        // std::cout << "[Baby Search] Use Last Grid = (" << p.x << ", " << p.y << ")" << std::endl;

        babyGhost->agent->SetTarget(p.x, p.y);
        babyGhost->lastPlayerGrid.valid = false; // 1회성
    }
}

void BabyGhost_Search::ChangeStateLogic()
{
    // 1. 회전 중 플레이어 발견 (Search 성공)
    if (phase == SearchPhase::RotateSearch && babyGhost->IsSeeing(babyGhost->GetAITarget()))
    {
        cout << "[BabyGhost_Search] Search Clear!! " << endl;
        babyGhost->ChangeState(BabyGhostState::Cry);
        return;
    }
    // 2. 회전 시간 종료 (Search 실패)
    if (phase == SearchPhase::RotateSearch && rotateTimer >= rotateTime)
    {
        cout << "[BabyGhost_Search] Search Fail.." << endl;
        babyGhost->ChangeState(BabyGhostState::Return); 
    }
}

void BabyGhost_Search::Update(float deltaTime)
{
    if (phase == SearchPhase::WaitBeforeMove)
    {
        waitTimer += deltaTime;
        if (waitTimer >= waitTime)
        {
            phase = SearchPhase::MoveToPoint;
        }
    }
}

void BabyGhost_Search::FixedUpdate(float deltaTime)
{
    if (phase == SearchPhase::RotateSearch)
    {
        rotateTimer += deltaTime;

        auto tr = babyGhost->GetOwner()->GetTransform();
        float newYaw = tr->GetYaw() + XMConvertToRadians(90.f) * deltaTime;

        tr->SetEuler(Vector3(0.f, newYaw, 0.f));
    }
}

void BabyGhost_Search::Exit()
{
    if (babyGhost->agent)
        babyGhost->agent->ClearTarget();
}