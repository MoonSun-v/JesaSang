#pragma once
#include "../Object/Component.h"
#include "../EngineSystem/GridSystem.h"

class CharacterControllerComponent;

class AgentComponent : public Component
{
    RTTR_ENABLE(Component)

public:
    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

    AgentComponent() = default;
    ~AgentComponent() = default;

public:
    bool externalControl = false; // FSM이 제어중인가? 

    CharacterControllerComponent* cct = nullptr;

    int cx = 0, cy = 0;             // 현재 위치 (중앙 기준)
    int targetCX = 0, targetCY = 0; // 목표 위치 (중앙 기준)
    bool hasTarget = false;

    float reachDist = 20.0f;  // 목표와의 거리 
    float patrolSpeed = 1.0f; // 상태마다 변경될 예정 

    bool isWaiting = false;   // 현재 대기 중인지
    float waitTimer = 0.0f;    // 남은 대기 시간
    float waitDuration = 0.0f; // 기본 대기 시간 (초) 

    std::vector<std::pair<int, int>> path; // A* 경로 저장 (그리드 좌표)

public:
    float giveWayTimer = 0.f;   // 양보 타이머
    float stuckTimer = 0.f;     // 정체 감지용
    Vector3 lastPos;            // 이전 위치

public:
    void OnInitialize() override;
    void OnStart() override;
    void OnFixedUpdate(float dt) override;

    // register enable
    void Enable_Inner() override;
    void Disable_Inner() override;

    void PickRandomTarget();
    void MoveAgent(const Vector3& dir, float speed, float dt);

    void SetWaitTime(float seconds);
    Vector3 ComputeSeparationForce(const Vector3& moveDir);
};