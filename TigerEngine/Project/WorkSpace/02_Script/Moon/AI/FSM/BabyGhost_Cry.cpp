#include "BabyGhost_Cry.h"

#include "EngineSystem/SceneSystem.h"
#include "../../../Ho/Sound/EnemySoundSource.h"

void BabyGhost_Cry::Enter()
{
    cout << "[BabyGhost_Cry] Enter Cry State" << endl;

    careTimer = 0.0f;
    caringAdult = nullptr; // 돌봐주는 유령 초기화
    adultArrived = false;

    babyGhost->animController->ChangeState("Cry");
    babyGhost->enemySound->PlaySound(EnemySoundType::BabyGhost_Crying_Sound);


    // 이동 완전 정지
    babyGhost->agent->externalControl = true;
    babyGhost->agent->path.clear();
}

void BabyGhost_Cry::ChangeStateLogic()
{
    // 돌봐주는 유령이 3초 이상 돌봐주면 복귀
    if (careTimer >= careDelay)
    {
        cout << "[BabyGhost_Cry] Care completed, returning to Patrol" << endl;
        
        // 아기 유령 복귀 
        babyGhost->ChangeState(BabyGhostState::Return);

        // 돌봐주는 유령 후속 행동 시작
        if (caringAdult)
        {
            caringAdult->StartPostBabyCare();
            caringAdult = nullptr;
        }
    }
}

void BabyGhost_Cry::Update(float deltaTime)
{
    auto cryPos = babyGhost->GetOwner()->GetTransform()->GetWorldPosition();

    // 돌봐주는 유령이 없으면 주변 Patrol 상태 어른 유령 찾기
    if (!caringAdult)
    {
        auto adultGhosts = SceneSystem::Instance().GetCurrentScene()->GetGameObjectsByName("Ghost_Adult");
        AdultGhostController* closestPatrolGhost = nullptr;
        float minDist = FLT_MAX; // 탐색 반경

        for (auto* go : adultGhosts)
        {
            auto* adult = go->GetComponent<AdultGhostController>();
            if (!adult) continue;
            if (adult->GetState() != AdultGhostState::Patrol) continue;

            float dist = (cryPos - adult->GetOwner()->GetTransform()->GetWorldPosition()).Length();
            if (dist < minDist)
            {
                minDist = dist;
                closestPatrolGhost = adult;
            }
        }

        if (closestPatrolGhost)
        {
            closestPatrolGhost->SetAITarget(babyGhost->GetOwner());
            closestPatrolGhost->chaseReason = ChaseReason::FromBabyCry;
            closestPatrolGhost->ChangeStateTo(AdultGhostState::Chase);
            caringAdult = closestPatrolGhost;

            cout << "[BabyGhost_Cry] Sending cry signal to one Patrol AdultGhost" << endl;
        }
    }

    // 돌봐주는 유령 근처에 도착하면 -> careTimer 증가 
    if (caringAdult)
    {
        float dist = (cryPos - caringAdult->GetOwner()->GetTransform()->GetWorldPosition()).Length();
        const float arriveThreshold = 200.0f; // 근접 기준
        if (dist <= arriveThreshold)
        {
            adultArrived = true;
        }

        if (adultArrived)
        {
            careTimer += deltaTime;
            cout << "[BabyGhost_Cry] Caring . . . " << careTimer << endl;
        }
    }
}


void BabyGhost_Cry::FixedUpdate(float deltaTime)
{
}

void BabyGhost_Cry::Exit()
{
    careTimer = 0.0f;
    babyGhost->agent->externalControl = false;

    // 돌봐주던 유령 초기화
    if (caringAdult)
    {
        caringAdult->SetAITarget(nullptr);
        caringAdult = nullptr;
    }
}