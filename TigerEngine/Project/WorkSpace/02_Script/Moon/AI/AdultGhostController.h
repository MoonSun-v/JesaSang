#pragma once
#include "Components/ScriptComponent.h"
#include "Components/AgentComponent.h"
#include "Components/GridComponent.h"
#include "Components/VisionComponent.h"
#include "Components/FBXRenderer.h"
#include "Components/FBXData.h"
#include "Components/AnimationController.h"

#include "Util/CollisionLayer.h"


class EnemySoundSource;
class IAdultGhostState;
class AdultGhost_Patrol;
class AdultGhost_Chase;
class AdultGhost_Search;
class AdultGhost_Return;
class AdultGhost_Attack;

enum class AdultGhostState
{
 // 순찰,   추격,   탐색,   복귀,   공격,    None
    Patrol, Chase, Search, Return, Attack, None
};

struct GridPos
{
public:
    int x = -1;
    int y = -1;
    bool valid = false;

    GridPos() = default;
    GridPos(int x_, int y_) : x(x_), y(y_), valid(true) {}
    GridPos(int x_, int y_, bool v_) : x(x_), y(y_), valid(v_) {}
};

// Search 상태의 진입 경로 
enum class SearchReason
{
    FromPatrol,   // 기척 or 함정 으로 넘어옴 
    FromChase,    // 추격 실패    으로 넘어옴 
    FromAttack,
    None
};

// Chase 상태의 진입 경로
enum class ChaseReason
{
    FromBabyCry,   
    None
};

class AdultGhostController : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);

    // Component 
    AgentComponent* agent = nullptr;
    VisionComponent* vision = nullptr;
    FBXRenderer* fbxRenderer = nullptr;
    FBXData* fbxData = nullptr;
    AnimationController* animController = nullptr;
    EnemySoundSource* enemySound = nullptr;

    // State
    AdultGhostState state = AdultGhostState::None;
    IAdultGhostState* currentState;
    IAdultGhostState* fsmStates[5];


    // HideObject tracking
    GameObject* curSeeingHideObject = nullptr;
    std::vector<GameObject*> hideObjects;
    bool hideLookRegistered = false;


    // Post BabyCare용
    float postCareTimer = 0.0f;
    bool postCareActive = false;
    Vector3 forcedTargetPos;


public:
    // AI가 처음 배치된 좌표 (추후 웨이 포인트)
    Vector3 initialPosition;

    
public: // Patrol Waypoints
    static const int MAX_PATROL_POINTS = 16;
    GridPos patrolPoints[MAX_PATROL_POINTS];
    int patrolPointCount = 0; // 실제 사용되는 개수 

private:
    int patrolIndex = 0;
    int lastVisitedWaypoint = -1;

private:
    // FSM
    void InitFSMStates();
    void ChangeState(AdultGhostState state);
    void LoadAnimation();

public:
    void OnStart() override;
    void OnUpdate(float delta) override;
    void OnFixedUpdate(float dt) override;
    void OnDestory() override;

    // Interaction
    void OnPlayerNoise(const Vector3& noiseWorldPos); // 플레이어에서 호출 
    void OnBabyCry(const Vector3& cryWorldPos);
    void OnAttackHit(); // 유령 충돌 오브젝트에서 호출

    // Helper
    void ResetAgentForMove(float speed);
    bool IsArrived() const;
    bool IsSeeing(GameObject* target) const;
    bool IsPlayerInSenseRange();
    bool IsPlayerHidden() const;
    bool CanAttackPlayer();
    bool CanDetectPlayer() const;
    bool CanKeepChase();
    void StartPostBabyCare();

    GameObject* GetAITarget() const;
    GameObject* GetPlayer() const;

    // 플레이어 발견 마지막 위치 (그리드 좌표) 
    GridPos lastPlayerGrid;

    // 상태의 진입 경로 (어떤 이유로 들어왔는가)
    SearchReason searchReason = SearchReason::None;
    ChaseReason  chaseReason = ChaseReason::None;

private:
    GameObject* target = nullptr;

public:
    // 외부에서 AdultGhost 상태를 가져오기
    AdultGhostState GetState() const { return state; }

    // 외부에서 AdultGhost 상태를 바꾸기
    void ChangeStateTo(AdultGhostState nextState)
    {
        ChangeState(nextState);
    }

    void SetAITarget(GameObject* newTarget);  // 외부에서 AdultGhost 타겟 지정
    GameObject* GetTarget() const { return target; } // 외부에서 타겟 확인

    // WayPoint 관련
    void SetNextPatrolTarget();
    void SetReturnToLastWaypoint();

public:
    // friend
    friend class IAdultGhostState;
    friend class AdultGhost_Patrol;
    friend class AdultGhost_Chase;
    friend class AdultGhost_Search;
    friend class AdultGhost_Return;
    friend class AdultGhost_Attack;
};
