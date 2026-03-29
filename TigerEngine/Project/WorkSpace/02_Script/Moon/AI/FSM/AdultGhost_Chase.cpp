#include "AdultGhost_Chase.h"
#include "AdultGhost_Search.h"

#include "EngineSystem/SceneSystem.h"


void AdultGhost_Chase::Enter()
{
    cout << "[AdultGhost_Chase] Enter Chase State" << endl;

    chaseTimer = 0.0f;
    repathTimer = 0.0f;
    sightCheckTimer = 0.0f;
    waitRotateTimer = 0.0f;
    waitMoveTimer = 0.0f; 

    adultGhost->ResetAgentForMove(4.0f); // Chase 속도
    adultGhost->animController->ChangeState("Idle");

    if (!adultGhost->target)
        adultGhost->target = adultGhost->GetPlayer();
    

    if (adultGhost->chaseReason == ChaseReason::FromBabyCry)
    {
        mode = ChaseMode::BabyCry_MoveToCryPoint;

        // 울었던 위치로 이동
        auto grid = GridSystem::Instance().GetMainGrid();
        if (grid)
        {
            int tx, ty;
            if (grid->WorldToGridFromCenter(adultGhost->forcedTargetPos, tx, ty))
            {
                adultGhost->agent->SetTarget(tx, ty);
            }
        }
    }
    else
    {
        mode = ChaseMode::Normal;
    }
}

void AdultGhost_Chase::ChangeStateLogic()
{
    if (!adultGhost->target) return;


    // 1. Hide 상태면 공격 안 함
    //if (adultGhost->IsPlayerHidden())
    //{
    //    adultGhost->ChangeState(AdultGhostState::Return);
    //    return;
    //}
    if (adultGhost->IsPlayerHidden())
    {
        adultGhost->searchReason = SearchReason::FromChase;
        adultGhost->ChangeState(AdultGhostState::Search);
        return;
    }

    // 2. 충돌이 아니어도 공격 가능 거리면 Attack
    if (adultGhost->CanAttackPlayer())
    {
        cout << "[AdultGhost_Chase] In attack range -> Attack\n";
        adultGhost->ChangeState(AdultGhostState::Attack);
        return;
    }

    if (adultGhost->state == AdultGhostState::Attack) return;
    if (adultGhost->postCareActive) return; // PostBabyCare 중이면 포기 금지 (Chase 유지)

    if (mode == ChaseMode::Normal)
    {
        if (sightCheckTimer >= sightCheckInterval)
        {
            sightCheckTimer = 0.0f;

            if (!adultGhost->CanKeepChase())
            {
                SaveLastPlayerGrid();
                adultGhost->searchReason = SearchReason::FromChase;
                adultGhost->ChangeState(AdultGhostState::Search);
                return;
            }
        }

        if (CanGiveUpChase())
        {
            SaveLastPlayerGrid();
            adultGhost->searchReason = SearchReason::FromChase;
            adultGhost->ChangeState(AdultGhostState::Search);
            return;
        }
    }
}

void AdultGhost_Chase::Update(float deltaTime)
{
    chaseTimer += deltaTime;
    repathTimer += deltaTime;
    sightCheckTimer += deltaTime;

    if (!adultGhost->target) return;


    switch (mode)
    {
    case ChaseMode::Normal:
        if (repathTimer >= repathInterval)
        {
            UpdateTargetGrid();
            repathTimer = 0.0f;
        }
        break;

    case ChaseMode::BabyCry_MoveToCryPoint:
        if (adultGhost->agent->IsArrived())
        {
            mode = ChaseMode::BabyCry_RotateWait;
            waitRotateTimer = 0.0f;
            adultGhost->agent->ClearTarget();
        }
        break;

    case ChaseMode::BabyCry_RotateWait:
        waitRotateTimer += deltaTime;
        if (waitRotateTimer >= waitRotateTime)
        {
            auto* player = adultGhost->GetPlayer();
            auto grid = GridSystem::Instance().GetMainGrid();
            if (player && grid)
            {
                int px, py;
                if (grid->WorldToGridFromCenter(player->GetTransform()->GetWorldPosition(), px, py))
                    adultGhost->agent->SetTarget(px, py);
            }

            mode = ChaseMode::BabyCry_ForceMoveToPlayer;
            waitMoveTimer = 0.0f;
        }
        break;

    case ChaseMode::BabyCry_ForceMoveToPlayer:
        waitMoveTimer += deltaTime;
        if (waitMoveTimer >= moveTime)
        {
            if (adultGhost->IsSeeing(adultGhost->target))
            {
                mode = ChaseMode::Normal;
                adultGhost->chaseReason = ChaseReason::None;
            }
            else
            {
                auto grid = GridSystem::Instance().GetMainGrid();
                if (grid)
                {
                    int px, py;
                    auto wp = adultGhost->target->GetTransform()->GetWorldPosition();
                    if (grid->WorldToGridFromCenter(wp, px, py))
                        adultGhost->lastPlayerGrid = { px, py, true };
                }

                adultGhost->searchReason = SearchReason::FromChase;
                adultGhost->chaseReason = ChaseReason::None;
                adultGhost->ChangeState(AdultGhostState::Search);
                return;
            }
        }
        break;
    }
}


void AdultGhost_Chase::FixedUpdate(float deltaTime)
{
    if (mode == ChaseMode::BabyCry_RotateWait)
    {
        auto tr = adultGhost->GetOwner()->GetTransform();
        float newYaw = tr->GetYaw() + XMConvertToRadians(90.f) * deltaTime;
        tr->SetEuler(Vector3(0.f, newYaw, 0.f));
    }
}

void AdultGhost_Chase::Exit()
{
    adultGhost->agent->ClearTarget();
}


// ------------------------------------------------------------------------- 

void AdultGhost_Chase::UpdateTargetGrid()
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    // [ 플레이어 위치 -> Grid ]
    int px, py;
    auto wp = adultGhost->target->GetTransform()->GetLocalPosition();
    if (!grid->WorldToGridFromCenter(wp, px, py)) return;

    int tx = px;
    int ty = py;

    if (!FindNearestWalkableCell(px, py, tx, ty))
    {
        cout << "[AdultGhost_Chase] No walkable cell near target ("
            << px << ", " << py << ")\n";
        return;
    }

    adultGhost->agent->SetTarget(tx, ty);

    cout << "[AdultGhost_Chase] ChaseTarget=("
        << tx << "," << ty << "), PlayerCell=(" << px << "," << py << ")\n";
}


// 추격 포기 조건
bool AdultGhost_Chase::CanGiveUpChase() const
{
    if (chaseTimer < minChaseTime)
        return false;

    // BabyCry에서 추격 중이면 포기 금지
    if (adultGhost->chaseReason == ChaseReason::FromBabyCry || adultGhost->postCareActive)
        return false;

    // 일반적인 경우 : 시야 밖이면 포기 
    return !adultGhost->IsSeeing(adultGhost->target);
}

void AdultGhost_Chase::SaveLastPlayerGrid()
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid || !adultGhost->target) return;

    int px, py;
    auto wp = adultGhost->target->GetTransform()->GetLocalPosition();
    if (grid->WorldToGridFromCenter(wp, px, py))
    {
        adultGhost->lastPlayerGrid = { px, py, true };
    }
}

bool AdultGhost_Chase::FindNearestWalkableCell(int targetX, int targetY, int& outX, int& outY)
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return false;

    // 1. 목표 칸 자체가 안 막혀 있으면 그대로 사용
    if (grid->IsWalkableFromCenter(targetX, targetY))
    {
        outX = targetX;
        outY = targetY;
        return true;
    }

    // 2. AI 현재 위치 구하기
    int myX, myY;
    auto myPos = adultGhost->GetOwner()->GetTransform()->GetWorldPosition();
    if (!grid->WorldToGridFromCenter(myPos, myX, myY))
        return false;

    // 3. 목표 칸 주변 반경 탐색
    // 반경 1 -> 2 -> 3 순서로 넓혀가면서, 이동 가능한 칸 중 AI와 가장 가까운 칸 선택
    for (int r = 1; r <= 3; ++r)
    {
        bool found = false;
        float bestScore = FLT_MAX;
        int bestX = -1;
        int bestY = -1;

        for (int y = targetY - r; y <= targetY + r; ++y)
        {
            for (int x = targetX - r; x <= targetX + r; ++x)
            {
                // 현재 반경의 테두리만 검사
                if (abs(x - targetX) != r && abs(y - targetY) != r)
                    continue;

                if (!grid->IsWalkableFromCenter(x, y))
                    continue;

                if (grid->IsOccupied(x, y))
                    continue;

                // AI 기준으로 가까운 칸 우선
                float score = float(abs(x - myX) + abs(y - myY));

                if (score < bestScore)
                {
                    bestScore = score;
                    bestX = x;
                    bestY = y;
                    found = true;
                }
            }
        }

        if (found)
        {
            outX = bestX;
            outY = bestY;
            return true;
        }
    }

    return false;
}