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

    Transform* transform = GetOwner()->GetTransform();
    Vector3 worldPos = transform->GetWorldPosition();

    // 시작 위치를 중앙 기준 grid 좌표로 변환
    if (!grid->WorldToGridFromCenter(worldPos, cx, cy))
    {
        std::cout << "[AgentComponent] Failed to convert world position to grid coord.\n";
        return;
    }

    // 시작 셀 점유 등록
    grid->Occupy(cx, cy, this);

    // stuck 판정용 이전 위치 저장
    lastWorldPos = worldPos;

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
    if (path.empty())
    {
        UpdatePath();

        // 경로 생성 실패 시 타겟 해제
        if (path.empty())
        {
            std::cout << "[AgentComponent] Path not found.\n";
            hasTarget = false;
            return;
        }
    }

    // 경로 따라 이동
    MoveAlongPath(dt);

    // 이동 후 정체 상태 확인
    DetectStuck(dt);
}

// ------------------------------------------------------
// External Control API(FSM에서 호출)
// ------------------------------------------------------

void AgentComponent::SetTarget(int x, int y)
{
    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    // 목표 셀이 막혀 있으면 타겟 설정하지 않음
    if (!grid->IsWalkableFromCenter(x, y))
    {
        std::cout << "[AgentComponent::SetTarget] Target blocked: (" << x << ", " << y << ")\n";
        return;
    }

    // 같은 타겟이면 무시
    if (hasTarget && targetCX == x && targetCY == y)
    {
        std::cout << "[AgentComponent::SetTarget] Same target ignored.\n";
        return;
    }


    targetCX = x;
    targetCY = y;

    hasTarget = true;
    arrived = false;

    // 새 타겟이 들어오면 상태를 초기화하고 경로를 다시 계산하도록 비움
    blockedTimer = 0.0f;
    stuckTimer = 0.0f;
    path.clear();
}

void AgentComponent::ClearTarget()
{
    hasTarget = false;
    arrived = false;
    path.clear();

    blockedTimer = 0.0f;
    stuckTimer = 0.0f;
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

    // 경로 첫 번째가 현재 셀이면 제거
    if (!path.empty() && path.front().cx == cx && path.front().cy == cy)
    {
        path.erase(path.begin());
    }

    std::cout << "\n=== Path Generated ===\n";
    std::cout << "Start      : (" << cx << ", " << cy << ")\n";
    std::cout << "Target     : (" << targetCX << ", " << targetCY << ")\n";
    std::cout << "Path Count : " << path.size() << "\n";

    if (!path.empty())
    {
        std::cout << "Next Step  : (" << path.front().cx << ", " << path.front().cy << ")\n";
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
    if (!grid || path.empty())
        return;

    const GridCoord nextCoord = path.front();

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
            path.clear();
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
        grid->Release(cx, cy);

        cx = nextCoord.cx;
        cy = nextCoord.cy;

        grid->Occupy(cx, cy, this);
        path.erase(path.begin());

        // 경로 소진 + 목표 셀 도착이면 이동 완료
        if (path.empty() && cx == targetCX && cy == targetCY)
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
            grid->Release(cx, cy);
            cx = newCX;
            cy = newCY;
            grid->Occupy(cx, cy, this);
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