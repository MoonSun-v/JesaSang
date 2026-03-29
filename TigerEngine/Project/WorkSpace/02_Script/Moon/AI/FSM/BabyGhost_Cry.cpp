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

    // 이동 멈추기
    babyGhost->agent->ClearTarget();
}

void BabyGhost_Cry::ChangeStateLogic()
{
    // 돌봐주는 유령이 3초 이상 돌봐주면 복귀
    if (careTimer >= careDelay)
    {
        cout << "[BabyGhost_Cry] Care completed -> Patrol" << endl;

        // 1. 먼저 Adult 후처리
        if (caringAdult)
        {
            caringAdult->StartPostBabyCare();
            caringAdult = nullptr;
        }

        // 2. 그 다음 Baby 상태 전환
        babyGhost->ChangeState(BabyGhostState::Patrol);
        return;
    }
}

void BabyGhost_Cry::Update(float deltaTime)
{
    auto cryPos = babyGhost->GetOwner()->GetTransform()->GetWorldPosition();

    // 돌봐주는 Adult 유령이 없으면 주변 Patrol 상태 어른 유령 찾기
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
            closestPatrolGhost->StartBabyCryChase(babyGhost->GetOwner());
            caringAdult = closestPatrolGhost;

            caringAdult = closestPatrolGhost;
            cout << "[BabyGhost_Cry] Sending cry signal to one Patrol AdultGhost" << endl;
        }
    }

    // 돌봐주는 유령 근처에 도착하면 -> careTimer 증가 
    if (caringAdult)
    {
        float dist = (cryPos - caringAdult->GetOwner()->GetTransform()->GetWorldPosition()).Length();
        const float arriveThreshold = 250.0f; // 근접 기준
        if (dist <= arriveThreshold)
        {
            adultArrived = true;
            careTimer += deltaTime;
            cout << "[BabyGhost_Cry] Caring... " << careTimer << endl;
        }
        else
        {
            // 멀어지면 다시 대기
            adultArrived = false;
            careTimer = 0.0f;
        }
    }
}


void BabyGhost_Cry::FixedUpdate(float deltaTime)
{
}

void BabyGhost_Cry::Exit()
{
    careTimer = 0.0f;
    adultArrived = false;

    // Exit에서는 Adult 상태를 건드리지 않음
    // caringAdult 정리는 ChangeStateLogic()에서 완료 시점에 처리
    caringAdult = nullptr;
}