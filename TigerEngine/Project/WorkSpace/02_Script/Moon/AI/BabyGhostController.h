#pragma once
#include "Components/ScriptComponent.h"
#include "Components/AgentComponent.h"
#include "Components/GridComponent.h"
#include "Components/VisionComponent.h"
#include "Components/FBXRenderer.h"
#include "Components/FBXData.h"
#include "Components/AnimationController.h"

class EnemySoundSource;
#include "Util/CollisionLayer.h"

class IBabyGhostState;
class BabyGhost_Patrol;
class BabyGhost_Search;
class BabyGhost_Return;
class BabyGhost_Cry;

enum class BabyGhostState
{
//  순찰,   탐색,    복귀,   울기,  None
    Patrol, Search, Return, Cry,   None
};

struct GridPos_Baby
{
    int x = -1;
    int y = -1;
    bool valid = false;
};

// Search 상태의 진입 경로 
enum class SearchReason_Baby
{
    FromPatrol,   // 기척 or 함정 으로 넘어옴 
    FromCry,    // 추격 실패    으로 넘어옴 
    None
};

class BabyGhostController : public ScriptComponent
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
    BabyGhostState state = BabyGhostState::None;
    IBabyGhostState* currentState;
    IBabyGhostState* fsmStates[4];

    // HideObject tracking
    GameObject* curSeeingHideObject = nullptr;
    std::vector<GameObject*> hideObjects;
    bool hideLookRegistered = false;

    // AI가 처음 배치된 좌표 (웨이 포인트)
    Vector3 initialPosition;

private:
    // FSM
    void InitFSMStates();
    void ChangeState(BabyGhostState state);

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

    // Helper
    void ResetAgentForMove(float speed);
    bool IsSeeing(GameObject* target) const;
    bool IsPlayerInSenseRange();

    GameObject* GetAITarget() const;
    GameObject* GetPlayer() const;


    // 플레이어 발견 마지막 위치 (그리드 좌표) 
    GridPos_Baby lastPlayerGrid;


    // 상태의 진입 경로 (어떤 이유로 들어왔는가)
    SearchReason_Baby searchReason = SearchReason_Baby::None;


public:
    // friend
    friend class IBabyGhostState;
    friend class BabyGhost_Patrol;
    friend class BabyGhost_Search;
    friend class BabyGhost_Return;
    friend class BabyGhost_Cry;
};
