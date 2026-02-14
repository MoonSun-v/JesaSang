#include "AdultGhost_Search.h"
#include "EngineSystem/SceneSystem.h"


void AdultGhost_Search::Enter()
{
    cout << "[AdultGhost_Search] Enter Search State" << endl;

    arrived = false;
    waitTimer = 0.0f;
    rotateTimer = 0.0f;

    adultGhost->ResetAgentForMove(3.5f);
    adultGhost->animController->ChangeState("Idle");

    phase = SearchPhase::WaitBeforeMove;

    // 1. Chase or Attack 에서 넘어온 경우 바로 이동
    if (adultGhost->searchReason == SearchReason::FromChase ||
        adultGhost->searchReason == SearchReason::FromAttack)
    {
        phase = SearchPhase::MoveToPoint;
    }

    // 2. 마지막 플레이어 위치 있으면 사용 
    if (adultGhost->lastPlayerGrid.valid)
    {
        auto& p = adultGhost->lastPlayerGrid;
        std::cout << "[Search] Use Last Grid = (" << p.x << ", " << p.y << ")" << std::endl;

        adultGhost->agent->targetCX = p.x;
        adultGhost->agent->targetCY = p.y;
        adultGhost->agent->hasTarget = true;

        adultGhost->lastPlayerGrid.valid = false; // 1회성
    }

    // MoveToPoint인데 타겟이 없으면 바로 실패 처리
    if (phase == SearchPhase::MoveToPoint && !adultGhost->agent->hasTarget)
    {
        adultGhost->ChangeState(AdultGhostState::Return);
        return;
    }

    cout << "[Search] phase=" << (int)phase
        << " hasTarget=" << adultGhost->agent->hasTarget
        << " reason=" << (int)adultGhost->searchReason << endl;
}

void AdultGhost_Search::ChangeStateLogic()
{
    // 1. 회전 중 플레이어 발견 (Search 성공)
    if (phase == SearchPhase::RotateSearch &&  adultGhost->IsSeeing(adultGhost->GetAITarget()))
    {
        cout << "[AdultGhost_Search] Search Clear!! " << endl;
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
    // 1. 어느 단계든 플레이어 감지
    if (adultGhost->IsSeeing(adultGhost->GetAITarget()))
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
            if (adultGhost->agent->hasTarget)
                phase = SearchPhase::MoveToPoint;
            else
                phase = SearchPhase::RotateSearch; // 타겟 없으면 바로 회전
        }
    }

    // 3. MoveToPoint 단계
    else if (phase == SearchPhase::MoveToPoint)
    {
        if (!adultGhost->agent->hasTarget)
        {
            phase = SearchPhase::RotateSearch;
            rotateTimer = 0.0f;
        }
    }

    //// 항상 플레이어 감지 체크
    //if (adultGhost->IsSeeing(adultGhost->GetAITarget()))
    //{
    //    cout << "[AdultGhost_Search] Player spotted during Search -> Chase!" << endl;
    //    adultGhost->ChangeState(AdultGhostState::Chase);
    //    return;
    //}

    //waitTimer += deltaTime;

    //if (phase == SearchPhase::WaitBeforeMove)
    //{
    //    if (waitTimer >= waitTime)
    //    {
    //        if (!adultGhost->agent->hasTarget)
    //        {
    //            cout << "[Search] Wait done but no target -> Return" << endl;
    //            adultGhost->ChangeState(AdultGhostState::Return);
    //            return;
    //        }
    //        phase = SearchPhase::MoveToPoint;
    //    }
    //}
}

void AdultGhost_Search::FixedUpdate(float deltaTime)
{
    //if (phase == SearchPhase::MoveToPoint && !adultGhost->agent->hasTarget)
    //{
    //    cout << "[Search] Target lost during Move -> RotateSearch" << endl;
    //    phase = SearchPhase::RotateSearch;
    //    rotateTimer = 0.0f;
    //    return;
    //}

    if (phase == SearchPhase::MoveToPoint && !arrived)
    {
        bool done = adultGhost->MoveToTarget(deltaTime);
        if (done)
        {
            arrived = true;
            phase = SearchPhase::RotateSearch;
            rotateTimer = 0.0f;

            // 도착 즉시 목표 정리
            adultGhost->agent->externalControl = true;
            adultGhost->agent->path.clear();
            adultGhost->agent->hasTarget = false;

            auto tr = adultGhost->GetOwner()->GetTransform();
            baseYaw = tr->GetYaw();
            targetYaw = baseYaw + XMConvertToRadians(360.f);

            adultGhost->agent->externalControl = true;
        }
    }
    else if (phase == SearchPhase::RotateSearch)
    {
        rotateTimer += deltaTime;

        auto tr = adultGhost->GetOwner()->GetTransform();
        float newYaw = tr->GetYaw() + XMConvertToRadians(90.f) * deltaTime;

        tr->SetEuler(Vector3(0.f, newYaw, 0.f));

        // 회전 완료 후 Return
        if (rotateTimer >= rotateTime)
        {
            adultGhost->ChangeState(AdultGhostState::Return);
        }
    }
}

void AdultGhost_Search::Exit()
{
    arrived = false;

    adultGhost->agent->externalControl = false;
    adultGhost->agent->path.clear();
    adultGhost->agent->hasTarget = false;
}