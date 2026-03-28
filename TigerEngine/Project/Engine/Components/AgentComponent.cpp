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
    Vector3 worldPos = tr->GetWorldPosition();

    if (!grid->WorldToGridFromCenter(worldPos, cx, cy))
    {
        std::cout << "[AgentComponent] Failed to map world to grid\n";
        return;
    }

    grid->Occupy(cx, cy, this); // 현재 위치 점유 추가

    lastWorldPos = worldPos;

    std::cout << "\n=== AgentComponent ===\n";
    std::cout << "Start CX,CY: " << cx << "," << cy << "\n";
    std::cout << "World Pos: " << worldPos.x << "," << worldPos.z << "\n";
    std::cout << "===================\n";
}

void AgentComponent::OnStart()
{
}

void AgentComponent::OnFixedUpdate(float dt)
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid || !cct) return;

    if (!hasTarget || arrived)
        return;

    if (path.empty())
    {
        UpdatePath();
        if (path.empty())
        {
            std::cout << "[Agent] Path not found\n";
            hasTarget = false;
            return;
        }
    }

    MoveAlongPath(dt);
    DetectStuck(dt); // 이동 후 체크 
}


void AgentComponent::SetTarget(int x, int y)
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    if (!grid->IsWalkableFromCenter(x, y))
    {
        std::cout << "[SetTarget] Target blocked: " << x << "," << y << "\n";
        return;
    }

    // 같은 타겟이면 무시
    if (hasTarget && targetCX == x && targetCY == y)
    {
        std::cout << "[SetTarget] SAME TARGET IGNORE\n";
        return;
    }

    targetCX = x;
    targetCY = y;

    hasTarget = true;
    arrived = false;

    blockedTimer = 0.0f;
    stuckTimer = 0.0f;
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

    // 현재 위치 제거
    if (!path.empty() && path[0].first == cx && path[0].second == cy)
    {
        path.erase(path.begin());
    }


    std::cout << "\n=== Path Generated ===\n";
    std::cout << "Start: " << cx << "," << cy << "\n";
    std::cout << "Target: " << targetCX << "," << targetCY << "\n";
    std::cout << "Path Size: " << path.size() << "\n";

    if (!path.empty())
    {
        std::cout << "First Node: "
            << path[0].first << "," << path[0].second << "\n";
    }
    else
    {
        std::cout << "[UpdatePath] EMPTY PATH\n";
    }

    std::cout << "=======================\n";
}

void AgentComponent::MoveAlongPath(float dt)
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid || path.empty())
        return;

    auto next = path.front();
    AgentComponent* occupier = grid->GetOccupier(next.first, next.second);

    // 다음 칸이 다른 AI에게 점유되어 있으면 잠시 기다렸다가 재탐색
    if (occupier && occupier != this)
    {
        blockedTimer += dt;

        if (blockedTimer > 0.3f)
        {
            std::cout << "[MoveAlongPath] Next tile blocked. Repath.\n";
            path.clear();
            blockedTimer = 0.0f;
        }
        return;
    }

    blockedTimer = 0.0f;

    Vector3 pos = GetOwner()->GetTransform()->GetWorldPosition();
    Vector3 targetPos = grid->GridToWorldFromCenter(next.first, next.second);

    Vector3 dir = targetPos - pos;
    dir.y = 0.0f;

    float dist = dir.Length();

    // 다음 셀 중심에 충분히 가까우면 해당 셀 도착 처리
    if (dist < reachDist)
    {
        grid->Release(cx, cy);

        cx = next.first;
        cy = next.second;

        grid->Occupy(cx, cy, this);
        path.erase(path.begin());

        if (path.empty() && cx == targetCX && cy == targetCY)
        {
            arrived = true;
            hasTarget = false;
            std::cout << "[MoveAlongPath] Arrived at target\n";
        }

        return;
    }

    if (dist > 0.001f)
    {
        dir.Normalize();
        MoveAgent(dir, moveSpeed, dt);
    }

    // 이동 후 실제 월드 위치 기반으로 grid 동기화
    Vector3 after = GetOwner()->GetTransform()->GetWorldPosition();

    int newCX, newCY;
    if (grid->WorldToGridFromCenter(after, newCX, newCY))
    {
        if (newCX != cx || newCY != cy)
        {
            grid->Release(cx, cy);
            cx = newCX;
            cy = newCY;
            grid->Occupy(cx, cy, this);
        }
    }
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
    if (!hasTarget || arrived)
    {
        stuckTimer = 0.0f;
        lastWorldPos = GetOwner()->GetTransform()->GetWorldPosition();
        return;
    }

    Vector3 currentPos = GetOwner()->GetTransform()->GetWorldPosition();
    float movedDist = (currentPos - lastWorldPos).Length();

    // 거의 안 움직였으면 stuck 누적
    if (movedDist < 0.5f)
    {
        stuckTimer += dt;

        if (stuckTimer > 1.0f)
        {
            std::cout << "[DetectStuck] Repath triggered\n";
            path.clear();
            stuckTimer = 0.0f;
        }
    }
    else
    {
        stuckTimer = 0.0f;
    }

    lastWorldPos = currentPos;
}