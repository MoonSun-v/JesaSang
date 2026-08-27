#pragma once
#include "../Object/Component.h"
#include "../EngineSystem/GridSystem.h"

#include "../Util/GridTypes.h"

class CharacterControllerComponent;

// -------------------------------------------------------------
// [ AgentComponent ]
// 
// - Grid 위에서 이동하는 AI 에이전트
// - 목표 좌표 설정
// - A* 경로 생성
// - 경로 추종 이동
// - 정체(stuck) / 차단(blocked) 처리
// -------------------------------------------------------------

class AgentComponent : public Component
{
    RTTR_ENABLE(Component)

public:
    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

    AgentComponent() = default;
    ~AgentComponent() = default;

public:
    // [ External Movement Dependency ]
    CharacterControllerComponent* cct = nullptr;


    // [ Current Grid Position ]
    int cx = 0;
    int cy = 0;


    // [ Movement Settings ]
    float reachDist = 20.0f;  // 다음 셀 중심에 도달했다고 보는 거리
    float moveSpeed = 1.0f;


private:
    // [ Target State ]
    int targetCX = 0;
    int targetCY = 0;
    bool hasTarget = false;
    bool arrived = false;


    // [ (A*) Path Data ] : 중앙 기준 그리드 좌표 
    std::vector<GridCoord> path;
    std::size_t pathIndex = 0;

    // [ Grid Occupancy State ]
    bool hasOccupiedCell = false;

    // [ Runtime Timer ]
    float stuckTimer = 0.0f;     // 거의 이동하지 못한 시간 누적
    float blockedTimer = 0.0f;   // 다음 셀이 점유되어 대기한 시간
    float pathRetryTimer = 0.0f; // 경로 탐색 실패 후 재시도까지 남은 시간
    Vector3 lastWorldPos;        // 이전 프레임 world position


public:
    void OnInitialize() override;
    void OnStart() override;
    void OnFixedUpdate(float dt) override;

    void Enable_Inner() override;
    void Disable_Inner() override;

public:
    // [ External Control API (FSM에서 호출) ]
    void SetTarget(int x, int y);
    void ClearTarget();

    bool HasTarget() const { return hasTarget; }
    bool IsArrived() const { return arrived; }
    bool IsStuck() const { return stuckTimer > 1.0f; }

    void SetSpeed(float s) { moveSpeed = s; }


private:
    // [ Internal Update Flow ]
    void UpdatePath();
    void MoveAlongPath(float dt);
    void DetectStuck(float dt);
    void MoveAgent(const Vector3& dir, float speed, float dt);

    bool HasRemainingPath() const { return pathIndex < path.size(); }
    void ClearPath()
    {
        path.clear();
        pathIndex = 0;
    }

    void OccupyCurrentCell();
    void ReleaseOccupiedCell();
};
