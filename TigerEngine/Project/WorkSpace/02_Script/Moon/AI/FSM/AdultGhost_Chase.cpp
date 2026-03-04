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

    // Agent 초기화 
    adultGhost->ResetAgentForMove(4.0f); // Chase 속도

    adultGhost->animController->ChangeState("Idle");

    // 타겟이 지정되어 있으면 그대로 사용
    if (!adultGhost->target)
    {
        adultGhost->target = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("AITarget");
    }

    // BabyCry에서 온 경우
    if (adultGhost->chaseReason == ChaseReason::FromBabyCry)
    {
        mode = ChaseMode::BabyCry;
        reachedCryPoint = false;
        waitRotateTimer = 0.0f;

        // 울었던 위치로 이동
        auto grid = GridSystem::Instance().GetMainGrid();
        if (grid)
        {
            int tx, ty;
            if (grid->WorldToGridFromCenter(adultGhost->forcedTargetPos, tx, ty))
            {
                adultGhost->agent->targetCX = tx;
                adultGhost->agent->targetCY = ty;
                adultGhost->agent->hasTarget = true;
                adultGhost->agent->path.clear();
            }
        }
    }
    else
    {
        mode = ChaseMode::Normal;
    }
    adultGhost->agent->externalControl = false;
}

void AdultGhost_Chase::ChangeStateLogic()
{
    if (!adultGhost->target) return;

    // 이미 Attack으로 바뀌었으면 로직 중단
    if (adultGhost->state == AdultGhostState::Attack) return;

    // PostBabyCare 중이면 포기 금지 (Chase 유지)
    if (adultGhost->postCareActive) return;

    // 3초마다 시야 체크
    if (sightCheckTimer >= sightCheckInterval)
    {
        sightCheckTimer = 0.0f;

        // 시야 밖이면 추격 실패 -> Search
        if (!adultGhost->IsSeeing(adultGhost->target))
        {
            // 마지막 본 위치 저장
            auto grid = GridSystem::Instance().GetMainGrid();
            if (grid)
            {
                int px, py;
                auto wp = adultGhost->target->GetTransform()->GetLocalPosition();
                if (grid->WorldToGridFromCenter(wp, px, py))
                {
                    adultGhost->lastPlayerGrid = { px, py, true };
                }
            }

            adultGhost->searchReason = SearchReason::FromChase;
            adultGhost->ChangeState(AdultGhostState::Search);
        }
    }

    // 추격 포기 조건 (최소 추격 시간 이후)
    if (mode != ChaseMode::BabyCry && CanGiveUpChase())
    {
        adultGhost->searchReason = SearchReason::FromChase;
        adultGhost->ChangeState(AdultGhostState::Search);
    }
}

void AdultGhost_Chase::Update(float deltaTime)
{
    chaseTimer += deltaTime;
    repathTimer += deltaTime;
    sightCheckTimer += deltaTime;

    // ----------------------------
    // PostBabyCare 처리
    // ----------------------------
    if (adultGhost->postCareActive)
    {
        adultGhost->postCareTimer += deltaTime; 

        // 5초 경과 시 PostBabyCare 종료
        if (adultGhost->postCareTimer >= careTime)
        {
            adultGhost->postCareActive = false;
            adultGhost->postCareTimer = 0.0f;

            GameObject* player = adultGhost->GetPlayer();
            if (player && adultGhost->IsSeeing(player))
            {
                adultGhost->SetAITarget(player);
                adultGhost->ChangeState(AdultGhostState::Chase);
            }
            else
            {
                adultGhost->ChangeState(AdultGhostState::Search);
            }
        }
        return;
    }


    // ----------------------------
    // 일반 Chase 처리
    // ----------------------------
    if (mode == ChaseMode::Normal)
    {
        if (!adultGhost->target) return;

        if (repathTimer >= repathInterval)  // 목표 위치가 변경되었는지 확인 (경로 갱신)
        {
            UpdateTargetGrid();
            repathTimer = 0.0f;
        }
    }
}


void AdultGhost_Chase::FixedUpdate(float deltaTime)
{
    // BabyCry 모드일 때
    if (mode == ChaseMode::BabyCry)
    {
        if (!reachedCryPoint)
        {
            // 울었던 지점으로 이동
            bool done = adultGhost->MoveToTarget(deltaTime);
            if (done)
            {
                reachedCryPoint = true;
                waitRotateTimer = 0.0f;
                adultGhost->agent->externalControl = true;
            }
        }
        else
        {
            // 회전 대기 중
            waitRotateTimer += deltaTime;
            auto tr = adultGhost->GetOwner()->GetTransform();
            tr->SetEuler(Vector3(0.f, tr->GetYaw() + XMConvertToRadians(90.f) * deltaTime, 0.f));

            if (waitRotateTimer >= waitRotateTime)
            {
                // 플레이어 위치로 5초간 이동 (시야 무시)
                auto grid = GridSystem::Instance().GetMainGrid();
                if (grid)
                {
                    int px, py;
                    auto player = adultGhost->GetPlayer();
                    if (player && grid->WorldToGridFromCenter(player->GetTransform()->GetLocalPosition(), px, py))
                    {
                        adultGhost->agent->targetCX = px;
                        adultGhost->agent->targetCY = py;
                        adultGhost->agent->hasTarget = true;
                        adultGhost->agent->path.clear();
                    }
                }
                adultGhost->agent->externalControl = false;

                mode = ChaseMode::MoveToLastSeen;
                waitMoveTimer = 0.0f;
            }
        }

        return;
    }

    // 5초 이동 모드
    if (mode == ChaseMode::MoveToLastSeen)
    {
        waitMoveTimer += deltaTime;

        // 5초 이동 후 Normal 모드로 전환
        if (waitMoveTimer >= moveTime)
        {
            mode = ChaseMode::Normal;
            waitMoveTimer = 0.0f;
        }
        adultGhost->MoveToTarget(deltaTime);
        return;
    }


    // Normal Chase는 MoveToTarget 계속 수행
    adultGhost->MoveToTarget(deltaTime);
}

void AdultGhost_Chase::Exit()
{
    adultGhost->agent->hasTarget = false;
    adultGhost->agent->externalControl = false;
    adultGhost->agent->path.clear();
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
    cout << "[AdultGhost_Chase] Player Grid = (" << px << "," << py << ")\n";

    // [ AI 위치 -> Grid ] : 디버그용
    auto myPos = adultGhost->GetOwner()->GetTransform()->GetWorldPosition();
    int cx, cy;
    grid->WorldToGridFromCenter(myPos, cx, cy);
    cout << "[AdultGhost_Chase] Ghost Grid = (" << cx << "," << cy << ")\n";

    cout << "[AdultGhost_Chase] Target Grid Set To = ("
        << adultGhost->agent->targetCX << ", " << adultGhost->agent->targetCY << ")\n";



    // [ 어느정도 이동했을 때 경로 갱신 ]
    int dist = abs(px - adultGhost->agent->targetCX)+ abs(py - adultGhost->agent->targetCY);

    const int repathThreshold = 2; // 2칸 이상 차이날 때만
    if (dist >= repathThreshold)
    {
        adultGhost->agent->targetCX = px;
        adultGhost->agent->targetCY = py;

        // 진행중인 경로가 거의 끝났을 때만 리셋
        if (adultGhost->agent->path.size() <= 2)
            adultGhost->agent->path.clear();

        adultGhost->agent->hasTarget = true;
    }
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