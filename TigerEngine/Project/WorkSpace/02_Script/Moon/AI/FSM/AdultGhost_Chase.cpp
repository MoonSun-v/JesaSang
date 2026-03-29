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
    // cout << "[AdultGhost_Chase] Player Grid = (" << px << "," << py << ")\n";

    adultGhost->agent->SetTarget(px, py);
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