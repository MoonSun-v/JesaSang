#include "AgentComponent.h"

#include "../Components/Transform.h"
#include "../Components/GridComponent.h"
#include "../Object/GameObject.h"
#include "../Util/JsonHelper.h"
#include "../EngineSystem/AgentSystem.h"

#include <algorithm>

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
    OccupyCurrentCell();
}

void AgentComponent::Disable_Inner()
{
    ReleaseOccupiedCell();
    AgentSystem::Instance().UnRegister(this);
}

// --------------------------------------------------------------------------- 


void AgentComponent::OnInitialize()
{
    cct = GetOwner()->GetComponent<CharacterControllerComponent>();
    if (!cct) return;

    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    OccupyCurrentCell();
    if (!hasOccupiedCell)
        return;

    Vector3 worldPos = GetOwner()->GetTransform()->GetWorldPosition();

    std::cout << "\n=== AgentComponent Initialize ===\n";
    std::cout << "Start Grid Coord : (" << cx << ", " << cy << ")\n";
    std::cout << "Start World Pos  : (" << worldPos.x << ", " << worldPos.z << ")\n";
    std::cout << "=================================\n";
}

void AgentComponent::OnStart()
{
}

void AgentComponent::OnFixedUpdate(float dt)
{
    GridComponent* grid = GridSystem::Instance().GetMainGrid();
    if (!grid || !cct)
        return;

    // 현재 이동할 타겟이 없거나 이미 도착 상태면 아무것도 하지 않음
    if (!hasTarget || arrived)
        return;

    // path가 비어 있으면 새 경로를 생성
    if (!HasRemainingPath())
    {
        // 현재 셀이 목표 셀이면 빈 경로도 정상 도착으로 처리
        if (cx == targetCX && cy == targetCY)
        {
            arrived = true;
            hasTarget = false;
            pathRetryTimer = 0.0f;
            return;
        }

        // 일시적인 점유로 경로를 찾지 못한 경우 일정 시간 후 다시 탐색
        if (pathRetryTimer > 0.0f)
        {
            pathRetryTimer = std::max(0.0f, pathRetryTimer - dt);
            return;
        }

        UpdatePath();

        // 경로 탐색에 실패해도 목표는 유지해 순찰 Agent가 영구 정지하지 않게 한다.
        if (!HasRemainingPath())
        {
            std::cout << "[AgentComponent] Path not found. Retry later.\n";
            pathRetryTimer = 0.3f;
            return;
        }

        pathRetryTimer = 0.0f;
    }

    // 경로 따라 이동
    MoveAlongPath(dt);

    // 이동 후 정체 상태 확인
    DetectStuck(dt);
}

// ------------------------------------------------------
// External Control API(FSM에서 호출)
// ------------------------------------------------------

bool AgentComponent::SetTarget(int x, int y)
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid)
        return false;

    // 목표 셀이 막혀 있으면 타겟 설정하지 않음
    if (!grid->IsWalkableFromCenter(x, y))
    {
        std::cout << "[AgentComponent::SetTarget] Target blocked: (" << x << ", " << y << ")\n";
        return false;
    }

    // 같은 타겟이면 무시
    if (hasTarget && targetCX == x && targetCY == y)
    {
        std::cout << "[AgentComponent::SetTarget] Same target ignored.\n";
        return true;
    }


    targetCX = x;
    targetCY = y;

    hasTarget = true;
    arrived = false;

    // 새 타겟이 들어오면 상태를 초기화하고 경로를 다시 계산하도록 비움
    blockedTimer = 0.0f;
    stuckTimer = 0.0f;
    pathRetryTimer = 0.0f;
    ClearPath();

    return true;
}

void AgentComponent::ClearTarget()
{
    hasTarget = false;
    arrived = false;
    ClearPath();

    blockedTimer = 0.0f;
    stuckTimer = 0.0f;
    pathRetryTimer = 0.0f;
}


// ------------------------------------------------------
// Path Update 
// ------------------------------------------------------
void AgentComponent::UpdatePath()
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    // 현재 위치 -> 목표 위치 경로 생성
    path = grid->FindPath(cx, cy, targetCX, targetCY);
    pathIndex = 0;

    // 경로 첫 번째가 현재 셀이면 삭제하지 않고 다음 인덱스부터 사용
    if (HasRemainingPath() && path[pathIndex].cx == cx && path[pathIndex].cy == cy)
        ++pathIndex;

    std::cout << "\n=== Path Generated ===\n";
    std::cout << "Start      : (" << cx << ", " << cy << ")\n";
    std::cout << "Target     : (" << targetCX << ", " << targetCY << ")\n";
    std::cout << "Path Count : " << (path.size() - pathIndex) << "\n";

    if (HasRemainingPath())
    {
        std::cout << "Next Step  : (" << path[pathIndex].cx << ", " << path[pathIndex].cy << ")\n";
    }
    else
    {
        std::cout << "[AgentComponent::UpdatePath] Empty path.\n";
    }
    std::cout << "=======================\n";
}


// ------------------------------------------------------
// Movement
// ------------------------------------------------------
void AgentComponent::MoveAlongPath(float dt)
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid || !HasRemainingPath())
        return;

    const GridCoord nextCoord = path[pathIndex];

    // 다음 셀을 이미 다른 agent가 점유하고 있는지 확인
    AgentComponent* occupier = grid->GetOccupier(nextCoord.cx, nextCoord.cy);

    // 다음 칸이 다른 AI에게 점유되어 있으면 잠시 기다렸다가 재탐색
    if (occupier && occupier != this)
    {
        blockedTimer += dt;

        // 일정 시간 이상 막혀 있으면 경로 재탐색 유도
        if (blockedTimer > 0.3f)
        {
            std::cout << "[AgentComponent::MoveAlongPath] Next cell blocked. Request repath.\n";
            ClearPath();
            blockedTimer = 0.0f;
        }
        return;
    }

    blockedTimer = 0.0f;

    Vector3 currentWorldPos = GetOwner()->GetTransform()->GetWorldPosition();
    Vector3 nextWorldPos = grid->GridToWorldFromCenter(nextCoord.cx, nextCoord.cy);

    Vector3 moveDir = nextWorldPos - currentWorldPos;
    moveDir.y = 0.0f;

    float distanceToNext = moveDir.Length();

    // 다음 셀 중심에 충분히 가까우면 해당 셀에 도착한 것으로 처리
    if (distanceToNext < reachDist)
    {
        ReleaseOccupiedCell();

        cx = nextCoord.cx;
        cy = nextCoord.cy;

        grid->Occupy(cx, cy, this);
        hasOccupiedCell = true;
        ++pathIndex;

        // 경로 소진 + 목표 셀 도착이면 이동 완료
        if (!HasRemainingPath() && cx == targetCX && cy == targetCY)
        {
            arrived = true;
            hasTarget = false;

            std::cout << "[AgentComponent::MoveAlongPath] Arrived at target.\n";
        }

        return;
    }

    // 아직 다음 셀에 도달하지 않았다면 계속 이동
    if (distanceToNext > 0.001f)
    {
        moveDir.Normalize();
        MoveAgent(moveDir, moveSpeed, dt);
    }

    // 이동 후 실제 월드 위치 기반으로 grid 동기화
    Vector3 after = GetOwner()->GetTransform()->GetWorldPosition();

    int newCX = 0;
    int newCY = 0;

    if (grid->WorldToGridFromCenter(after, newCX, newCY))
    {
        if (newCX != cx || newCY != cy)
        {
            ReleaseOccupiedCell();
            cx = newCX;
            cy = newCY;
            grid->Occupy(cx, cy, this);
            hasOccupiedCell = true;
        }
    }
}


void AgentComponent::MoveAgent(const Vector3& dir, float speed, float dt)
{
    // 1. CharacterController를 통해 실제 이동
    cct->MoveAI(dir, speed, dt);

    // 2. 이동 방향을 바라보도록 Y축 회전 보정
    if (dir.LengthSquared() > 0.0001f) // 거의 0이 아니면
    {
        auto transform = GetOwner()->GetTransform();

        // 이동 방향의 Y축 회전 계산
        float targetYaw = atan2f(-dir.x, -dir.z); // Z-forward 기준
        float currentYaw = transform->GetYaw();

        // 회전 
        float rotationSpeed = 5.0f; // 회전 속도
        float newYaw = currentYaw + (targetYaw - currentYaw) * std::min(dt * rotationSpeed, 1.0f);

        transform->SetRotationY(newYaw);
    }
}


// ------------------------------------------------------
// Stuck Detection
// ------------------------------------------------------
void AgentComponent::DetectStuck(float dt)
{
    // 현재 이동 중이 아니면 stuck 상태를 누적하지 않음
    if (!hasTarget || arrived)
    {
        stuckTimer = 0.0f;
        lastWorldPos = GetOwner()->GetTransform()->GetWorldPosition();
        return;
    }

    Vector3 currentPos = GetOwner()->GetTransform()->GetWorldPosition();
    float movedDistance = (currentPos - lastWorldPos).Length();

    // 거의 움직이지 못했으면 stuck 시간 누적
    if (movedDistance < 0.5f)
    {
        stuckTimer += dt;

        // 일정 시간 이상 제자리면 경로 재탐색 유도
        if (stuckTimer > 1.0f)
        {
            std::cout << "[AgentComponent::DetectStuck] Repath triggered.\n";
            ClearPath();
            stuckTimer = 0.0f;
        }
    }
    else
    {
        stuckTimer = 0.0f;
    }

    lastWorldPos = currentPos;
}


// ------------------------------------------------------
// Grid Occupancy
// ------------------------------------------------------
void AgentComponent::OccupyCurrentCell()
{
    GridComponent* grid = GridSystem::Instance().GetMainGrid();
    if (!grid || !GetOwner() || !GetOwner()->GetTransform())
        return;

    Vector3 worldPos = GetOwner()->GetTransform()->GetWorldPosition();

    int currentCX = 0;
    int currentCY = 0;
    if (!grid->WorldToGridFromCenter(worldPos, currentCX, currentCY))
    {
        std::cout << "[AgentComponent] Failed to convert world position to grid coord.\n";
        return;
    }

    // 이미 다른 Agent가 점유한 셀을 덮어쓰지 않는다.
    AgentComponent* occupier = grid->GetOccupier(currentCX, currentCY);
    if (occupier && occupier != this)
    {
        std::cout << "[AgentComponent] Current grid cell is already occupied.\n";
        return;
    }

    cx = currentCX;
    cy = currentCY;
    grid->Occupy(cx, cy, this);
    hasOccupiedCell = true;
    lastWorldPos = worldPos;
}

void AgentComponent::ReleaseOccupiedCell()
{
    if (!hasOccupiedCell)
        return;

    GridComponent* grid = GridSystem::Instance().GetMainGrid();
    if (grid && grid->GetOccupier(cx, cy) == this)
        grid->Release(cx, cy);

    hasOccupiedCell = false;
}
