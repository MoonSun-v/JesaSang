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
    int x = -1;
    int y = -1;
    bool valid = false;
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


    // AI가 처음 배치된 좌표 (웨이 포인트)
    Vector3 initialPosition;


    // Post BabyCare용
    float postCareTimer = 0.0f;
    bool postCareActive = false;
    Vector3 forcedTargetPos;


private:
    // FSM
    void InitFSMStates();
    void ChangeState(AdultGhostState state);

    // Animation
    void LoadAnimation();

    // Movement (공통)
    bool MoveToTarget(float delta);
    void RotateByDirection(const Vector3& moveDir, float delta);

public:
    void OnStart() override;
    void OnUpdate(float delta) override;
    void OnFixedUpdate(float dt) override;
    void OnDestory() override;

    // Interaction
    void OnPlayerNoise(const Vector3& noiseWorldPos); // 플레이어에서 호출 
    void OnAttackHit(); // 유령 충돌 오브젝트에서 호출

    // Helper
    void ResetAgentForMove(float speed);
    // void SetAITarget(GameObject* newTarget);
    bool IsSeeing(GameObject* target) const;
    bool IsPlayerInSenseRange();
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

    // 외부에서 AdultGhost 타겟 지정
    void SetAITarget(GameObject* newTarget)
    {
        if (!agent) return;

        target = newTarget;    
        agent->path.clear();
        agent->hasTarget = true;
    }

    // 외부에서 타겟 확인
    GameObject* GetTarget() const { return target; }

public:
    // friend
    friend class IAdultGhostState;
    friend class AdultGhost_Patrol;
    friend class AdultGhost_Chase;
    friend class AdultGhost_Search;
    friend class AdultGhost_Return;
    friend class AdultGhost_Attack;
};
