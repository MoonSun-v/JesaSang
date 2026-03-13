#include "AgentComponent.h"
#include "../Components/Transform.h"
#include "../Components/GridComponent.h"
#include "../Object/GameObject.h"
#include "../Util/JsonHelper.h"
#include "../EngineSystem/AgentSystem.h"
#include <random>

RTTR_REGISTRATION
{
    rttr::registration::class_<AgentComponent>("AgentComponent")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr)

        .property("reachDist", &AgentComponent::reachDist)
        .property("patrolSpeed", &AgentComponent::patrolSpeed);
}

nlohmann::json AgentComponent::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void AgentComponent::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void AgentComponent::Enable_Inner()
{
    AgentSystem::Instance().Register(this);
}

void AgentComponent::Disable_Inner()
{
    AgentSystem::Instance().UnRegister(this);
}

// --------------------------------------------------------------------------- 


void AgentComponent::OnInitialize()
{
    cct = GetOwner()->GetComponent<CharacterControllerComponent>();
    if (!cct) return;

    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    auto tr = GetOwner()->GetTransform();
    grid->WorldToGridFromCenter(tr->GetWorldPosition(), cx, cy);
    grid->Occupy(cx, cy, this); // 현재 위치 점유 추가
    lastPos = tr->GetWorldPosition();
}

void AgentComponent::OnStart()
{
}

// -----------------------------------------
// 무작위 목표(Grid 좌표)를 선택하는 함수
// -----------------------------------------
void AgentComponent::PickRandomTarget()
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;
      
    int range = 10; // 현재 위치(cx, cy)를 기준으로 목표를 선택할 범위
     
    hasTarget = false;

    static std::random_device rd; 
    static std::mt19937 gen(rd()); // 메르센 트위스터 난수 생성기
    std::uniform_int_distribution<> dis(-range, range); // -range ~ +range 범위의 균등 정수 분포

    for (int i = 0; i < 20; ++i) // 최대 20번 
    {
        // 현재 좌표(cx, cy) 기준으로 랜덤한 좌표(rx, ry) 생성
        int rx = cx + dis(gen);
        int ry = cy + dis(gen);

        // 생성된 좌표가 이동 가능한지 체크
        if (grid->IsWalkableFromCenter(rx, ry))
        {
            targetCX = rx; // 목표 X 좌표 설정
            targetCY = ry; // 목표 Y 좌표 설정
            hasTarget = true;
            return;
        }
    }

    std::cout << "[AgentComponent] No valid target found\n";
}

void AgentComponent::OnFixedUpdate(float dt)
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    // FSM이 직접 path/target을 관리
    if (externalControl) { /*std::cout << "[AgentComponent] FSM is Used path/target! \n";*/ return; }

    // 양보 중이면 아무것도 안함
    if (giveWayTimer > 0.f)
    {
        giveWayTimer -= dt;
        // std::cout << "[AgentComponent] giveWayTimer ing \n"; 
        return; 
    }

    // 정체 감지
    Vector3 currentPos = GetOwner()->GetTransform()->GetWorldPosition();

    if ((currentPos - lastPos).Length() < 1.0f)
    {
        stuckTimer += dt;

        if (stuckTimer > 1.0f)   // 1초 이상 못 움직이면
        {
            path.clear();
            hasTarget = false;   // 강제 재탐색
            stuckTimer = 0.f;
        }
    }
    else
    {
        stuckTimer = 0.f;
    }

    lastPos = currentPos;


    // 대기 중이면 시간 감소 
    if (isWaiting)
    {
        waitTimer -= dt;
        if (waitTimer <= 0.f)
        {
            isWaiting = false;
            PickRandomTarget(); // 다시 탐색 시작
        }
        std::cout << "[AgentComponent] isWaiting is true \n";
        return; // 대기 중엔 이동 안함
    }


    // 목표가 없으면 새 목표 선택
    if (!hasTarget)
    {
        PickRandomTarget();
        if (!hasTarget) { std::cout << "[AgentComponent] Target is Null \n"; return; } // 목표 선택 실패하면 종료
    }

    // 경로 없으면 새 경로 생성
    if (path.empty())
    {
        path = grid->FindPath(cx, cy, targetCX, targetCY);
        if (path.empty())
        {
            // 경로를 못찾으면 목표 재선택
            PickRandomTarget();
            path = grid->FindPath(cx, cy, targetCX, targetCY);

            // 여전히 경로 없으면 이동 안함
            if (path.empty())
            {
                std::cout << "[AgentComponent] still Target is Null \n"; return;
            } 
        }
    }

    auto next = path.front();

    // 점유자 확인
    AgentComponent* blocker = nullptr;
    if (grid->IsOccupied(next.first, next.second))
    {
        // 점유자 얻기 (Grid에 GetOccupier 함수 추가 필요)
        blocker = grid->GetOccupier(next.first, next.second);
    }

    if (blocker && blocker != this)
    {
        // 스왑 상황 감지
        if (!blocker->path.empty())
        {
            auto blockerNext = blocker->path.front();

            if (blockerNext.first == cx && blockerNext.second == cy)
            {
                // 서로 자리 바꾸려는 상황

                // 간단 우선순위 (포인터 주소 기준)
                if (this < blocker)
                {
                    giveWayTimer = 0.5f;  // 0.5초 양보
                    return;
                }
            }
        }

        // 그냥 막힌 상황이면 재탐색
        // std::cout << "[AgentComponent] Path is Block \n";
        path.clear();
        return;
    }

    Vector3 targetPos = grid->GridToWorldFromCenter(next.first, next.second);

    Vector3 pos = GetOwner()->GetTransform()->GetWorldPosition();
    Vector3 dir = targetPos - pos;
    dir.y = 0;

    if (dir.Length() < reachDist)
    {
        // [ 이동 성공 시, 점유 갱신 ]
        
        // 다음 셀이 이미 점유 중이면 대기 후 재탐색
        if (grid->IsOccupied(next.first, next.second))
        {
            path.clear();   // 재탐색 유도
            return;
        }

        grid->Release(cx, cy); // 기존 셀 해제

        cx = next.first;
        cy = next.second;

        grid->Occupy(cx, cy, this); // 새 셀 점유

        path.erase(path.begin());


        // [ 경로가 다 끝나면 잠시 멈췄다가 목표 재선택 ]
        if (path.empty())
        {
            isWaiting = true;
            waitTimer = waitDuration;
            hasTarget = false;  // 다음 탐색은 대기 후
            return;
        }
    }
    else
    {
        dir.Normalize();

        // 주변 Agent 회피
        Vector3 sep = ComputeSeparationForce(dir);
        Vector3 finalDir = dir + sep;
        float len = finalDir.Length();
        if (len > 0.0001f)
        {
            finalDir.x /= len;
            finalDir.y /= len;
            finalDir.z /= len;
        }

        MoveAgent(finalDir, patrolSpeed, dt);
    }

    // 디버그 출력
    std::cout << "[AgentComponent] Current: (" << cx << "," << cy << ") "
        << "Next: (" << next.first << "," << next.second << ") "
        << "Path size: " << path.size() << std::endl;
}


void AgentComponent::MoveAgent(const Vector3& dir, float speed, float dt)
{
    if(!cct)
    {
        std::cout << "[AgentComponent] MoveAgent, cct is NULL." << std::endl;
        return;
    }

    // 1. 이동
    cct->MoveAI(dir, speed, dt);

    // 2. 이동 방향으로 회전 (Y축 기준)
    if (dir.LengthSquared() > 0.0001f) // 거의 0이 아니면
    {
        auto tr = GetOwner()->GetTransform();

        // 이동 방향의 Y축 회전 계산
        float targetYaw = atan2f(-dir.x, -dir.z); // Z-forward 기준
        float currentYaw = tr->GetYaw();

        // 회전 
        float rotationSpeed = 5.0f; // 회전 속도
        float newYaw = currentYaw + (targetYaw - currentYaw) * std::min(dt * rotationSpeed, 1.0f);

        tr->SetRotationY(newYaw);
    }
}

void AgentComponent::SetWaitTime(float seconds)
{
    waitDuration = seconds;
}


Vector3 AgentComponent::ComputeSeparationForce(const Vector3& moveDir)
{
    Vector3 force(0, 0, 0);
    auto& agents = AgentSystem::Instance().GetAgents();

    Vector3 myPos = GetOwner()->GetTransform()->GetWorldPosition();

    const float radius = 90.0f;   // 유령 크기 + 안전 거리
    const float sideBias = 0.5f;   // 옆으로 비켜가는 힘
    const float forwardBias = 0.1f;

    for (auto* other : agents)
    {
        if (other == this) continue;

        Vector3 otherPos = other->GetOwner()->GetTransform()->GetWorldPosition();
        Vector3 toMe = myPos - otherPos;
        toMe.y = 0;

        float dist = toMe.Length();
        if (dist < radius && dist > 1.0f)
        {
            toMe.Normalize();

            // 진행방향 기준 좌우 벡터
            Vector3 right(-moveDir.z, 0, moveDir.x);

            // 상대가 왼쪽인지 오른쪽인지
            // float side = Vector3::Dot(right, toMe);
            float side = right.x * toMe.x + right.y * toMe.y + right.z * toMe.z;

            Vector3 sideForce = right * side * sideBias;
            Vector3 backForce = toMe * forwardBias;

            float t = (radius - dist) / radius; // 가까울수록 강함
            force += (sideForce + backForce) * t;
        }
    }

    return force;
}
