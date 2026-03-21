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
    CharacterControllerComponent* cct = nullptr;

    // 현재 위치 (중앙 기준)
    int cx = 0, cy = 0;             

    // 이동 
    float reachDist = 20.0f;  // 목표와의 거리 
    float moveSpeed = 1.0f;

private:
    // 목표 위치 (중앙 기준)
    int targetCX = 0, targetCY = 0; 
    bool hasTarget = false;

    // A* 경로 저장 (그리드 좌표)
    std::vector<std::pair<int, int>> path; 

    //bool isWaiting = false;   // 현재 대기 중인지
    //float waitTimer = 0.0f;    // 남은 대기 시간
    //float waitDuration = 0.0f; // 기본 대기 시간 (초) 

    // 정체 감지
    float stuckTimer = 0.0f;     
    Vector3 lastPos;            // 이전 위치

    // 상태
    bool arrived = false;

public:
    // float giveWayTimer = 0.f;   // 양보 타이머
   

public:
    void OnInitialize() override;
    void OnStart() override;
    void OnFixedUpdate(float dt) override;

    // register enable
    void Enable_Inner() override;
    void Disable_Inner() override;

    // FSM 인터페이스-
    void SetTarget(int x, int y);
    void ClearTarget();

    bool HasTarget() const { return hasTarget; }
    bool IsArrived() const { return arrived; }
    bool IsStuck() const { return stuckTimer > 1.0f; }

    void SetSpeed(float s) { moveSpeed = s; }

private:
    void UpdatePath();
    void MoveAlongPath(float dt);
    void DetectStuck(float dt);
    
    void MoveAgent(const Vector3& dir, float speed, float dt);

    // void PickRandomTarget();

    //void SetWaitTime(float seconds);
    //Vector3 ComputeSeparationForce(const Vector3& moveDir);
};