#include "BabyGhost_Return.h"

void BabyGhost_Return::Enter()
{
    cout << "[BabyGhost_Return] Enter Return State" << endl;

    babyGhost->ResetAgentForMove(6.0f); // Return 속도 
    visionTimer = 0.0f;
    babyGhost->animController->ChangeState("Idle");

    // 가장 가까운 순찰 포인트로 복귀
    babyGhost->SetReturnToNearestPatrolTarget();
}

void BabyGhost_Return::ChangeStateLogic()
{ 
    // 3초 후 부터 시야에 플레이어 감지 (바로 감지하는거 방지하기 위해)
    if (visionTimer >= visionDelay)
    {
        auto* player = babyGhost->GetAITarget();
        if (player && babyGhost->IsSeeing(player))
        {
            cout << "[BabyGhost_Return] Player detected -> Cry" << endl;
            babyGhost->ChangeState(BabyGhostState::Cry);
            return;
        }
    }

    // 2. 도착 체크 : IsArrived()로 체크 (웨이포인트 좌표로 이동 완료 여부)
    if (babyGhost->agent->IsArrived())
    {
        // Return으로 도착한 현재 waypoint는 이미 밟았으니
        // Patrol은 다음 waypoint부터 시작하게 넘김
        if (babyGhost->patrolPointCount > 0)
        {
            babyGhost->patrolIndex++;
            if (babyGhost->patrolIndex >= babyGhost->patrolPointCount)
                babyGhost->patrolIndex = 0;
        }

        babyGhost->ChangeState(BabyGhostState::Patrol);
        return;
    }
}

void BabyGhost_Return::Update(float deltaTime)
{
    visionTimer += deltaTime;
}

void BabyGhost_Return::FixedUpdate(float deltaTime)
{
}

void BabyGhost_Return::Exit()
{
    babyGhost->agent->ClearTarget();
}