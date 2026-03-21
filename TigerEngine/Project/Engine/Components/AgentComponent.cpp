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
        .property("moveSpeed", &AgentComponent::moveSpeed);
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

void AgentComponent::OnFixedUpdate(float dt)
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid || !cct) return;

    DetectStuck(dt);

    if (!hasTarget)
        return;

    if (arrived)
        return;

    if (path.empty())
    {
        UpdatePath();
        if (path.empty())
        {
            // 경로 못 찾으면 실패 처리
            hasTarget = false;
            return;
        }
    }

    MoveAlongPath(dt);
}


void AgentComponent::SetTarget(int x, int y)
{
    targetCX = x;
    targetCY = y;

    hasTarget = true;
    arrived = false;
    path.clear();
}

void AgentComponent::ClearTarget()
{
    hasTarget = false;
    path.clear();
}


void AgentComponent::UpdatePath()
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    path = grid->FindPath(cx, cy, targetCX, targetCY);
}

void AgentComponent::MoveAlongPath(float dt)
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid || path.empty()) return;

    auto next = path.front();

    Vector3 targetPos = grid->GridToWorldFromCenter(next.first, next.second);
    Vector3 pos = GetOwner()->GetTransform()->GetWorldPosition();

    Vector3 dir = targetPos - pos;
    dir.y = 0;

    if (dir.Length() < reachDist)
    {
        // 도착 → 그리드 갱신
        grid->Release(cx, cy);

        cx = next.first;
        cy = next.second;

        grid->Occupy(cx, cy, this);

        path.erase(path.begin());

        if (path.empty())
        {
            arrived = true;
            hasTarget = false;
        }

        return;
    }

    dir.Normalize();
    MoveAgent(dir, moveSpeed, dt);
}

void AgentComponent::MoveAgent(const Vector3& dir, float speed, float dt)
{
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

void AgentComponent::DetectStuck(float dt)
{
    Vector3 currentPos = GetOwner()->GetTransform()->GetWorldPosition();

    if ((currentPos - lastPos).Length() < 1.0f)
    {
        stuckTimer += dt;

        if (stuckTimer > 1.0f)
        {
            path.clear(); // 재탐색 유도
        }
    }
    else
    {
        stuckTimer = 0.f;
    }

    lastPos = currentPos;
}


// --------------------------------------------------------------

//void AgentComponent::SetWaitTime(float seconds)
//{
//    waitDuration = seconds;
//}

//Vector3 AgentComponent::ComputeSeparationForce(const Vector3& moveDir)
//{
//    Vector3 force(0, 0, 0);
//    auto& agents = AgentSystem::Instance().GetAgents();
//
//    Vector3 myPos = GetOwner()->GetTransform()->GetWorldPosition();
//
//    const float radius = 90.0f;   // 유령 크기 + 안전 거리
//    const float sideBias = 0.5f;   // 옆으로 비켜가는 힘
//    const float forwardBias = 0.1f;
//
//    for (auto* other : agents)
//    {
//        if (other == this) continue;
//
//        Vector3 otherPos = other->GetOwner()->GetTransform()->GetWorldPosition();
//        Vector3 toMe = myPos - otherPos;
//        toMe.y = 0;
//
//        float dist = toMe.Length();
//        if (dist < radius && dist > 1.0f)
//        {
//            toMe.Normalize();
//
//            // 진행방향 기준 좌우 벡터
//            Vector3 right(-moveDir.z, 0, moveDir.x);
//
//            // 상대가 왼쪽인지 오른쪽인지
//            // float side = Vector3::Dot(right, toMe);
//            float side = right.x * toMe.x + right.y * toMe.y + right.z * toMe.z;
//
//            Vector3 sideForce = right * side * sideBias;
//            Vector3 backForce = toMe * forwardBias;
//
//            float t = (radius - dist) / radius; // 가까울수록 강함
//            force += (sideForce + backForce) * t;
//        }
//    }
//
//    return force;
//}

// -----------------------------------------
// 무작위 목표(Grid 좌표)를 선택하는 함수
// -----------------------------------------
//void AgentComponent::PickRandomTarget()
//{
//    auto grid = GridSystem::Instance().GetMainGrid();
//    if (!grid) return;
//
//    int range = 10; // 현재 위치(cx, cy)를 기준으로 목표를 선택할 범위
//
//    hasTarget = false;
//
//    static std::random_device rd;
//    static std::mt19937 gen(rd()); // 메르센 트위스터 난수 생성기
//    std::uniform_int_distribution<> dis(-range, range); // -range ~ +range 범위의 균등 정수 분포
//
//    for (int i = 0; i < 20; ++i) // 최대 20번 
//    {
//        // 현재 좌표(cx, cy) 기준으로 랜덤한 좌표(rx, ry) 생성
//        int rx = cx + dis(gen);
//        int ry = cy + dis(gen);
//
//        // 생성된 좌표가 이동 가능한지 체크
//        if (grid->IsWalkableFromCenter(rx, ry))
//        {
//            targetCX = rx; // 목표 X 좌표 설정
//            targetCY = ry; // 목표 Y 좌표 설정
//            hasTarget = true;
//            return;
//        }
//    }
//
//    std::cout << "[AgentComponent] No valid target found\n";
//}