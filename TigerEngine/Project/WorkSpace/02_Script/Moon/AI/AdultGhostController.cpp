#include "AdultGhostController.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

#include "Object/GameObject.h"
#include "EngineSystem/SceneSystem.h"

#include "FSM/IAdultGhostState.h"
#include "FSM/AdultGhost_Patrol.h"
#include "FSM/AdultGhost_Chase.h"
#include "FSM/AdultGhost_Search.h"
#include "FSM/AdultGhost_Attack.h"
#include "FSM/AdultGhost_Return.h"

#include "../../Woo/Object/HideObject.h"
#include "../../Woo/Player/PlayerController.h"
#include "../../Ho/Sound/EnemySoundSource.h"


REGISTER_COMPONENT(AdultGhostController)

RTTR_REGISTRATION
{
    rttr::registration::class_<AdultGhostController>("AdultGhostController")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}



nlohmann::json AdultGhostController::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void AdultGhostController::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

// Util 
float WrapAngleRad(float a)
{
    while (a > XM_PI)  a -= XM_2PI;
    while (a < -XM_PI) a += XM_2PI;
    return a;
}

// -----------------------------------------------------------
// [ Process ]
// -----------------------------------------------------------

void AdultGhostController::OnStart()
{
    agent = GetOwner()->GetComponent<AgentComponent>();
    vision = GetOwner()->GetComponent<VisionComponent>();
    fbxRenderer = GetOwner()->GetComponent<FBXRenderer>();
    fbxData = GetOwner()->GetComponent<FBXData>();
    animController = GetOwner()->GetComponent<AnimationController>();
    enemySound = GetOwner()->GetComponent<EnemySoundSource>();

    if (!agent || !vision || !fbxRenderer || !fbxData || !animController)
    {
        std::cout << "[AdultGhostController] Component Missing" << std::endl;
        return;
    }

    // Hide Object 모두 수집 
    hideObjects = SceneUtil::GetObjectsByName("HideObject");

    LoadAnimation();  

    // 최초 시작 위치 저장
    initialPosition = GetOwner()->GetTransform()->GetWorldPosition(); // local X

    InitFSMStates();
    ChangeState(AdultGhostState::Patrol);
}

void AdultGhostController::OnUpdate(float delta)
{
    if (!currentState) return;

    currentState->ChangeStateLogic();
    currentState->Update(delta);
}

void AdultGhostController::OnFixedUpdate(float dt)
{
    if (currentState)
    {
        currentState->FixedUpdate(dt);
    }
}

void AdultGhostController::OnDestory()
{
    // AI가 HideObject를 보고 있다가 삭제 되었을 때
    if (hideLookRegistered && curSeeingHideObject)
    {
        if (auto* hide = curSeeingHideObject->GetComponent<HideObject>())
            hide->UnregisterAILook(this);
    }
    hideLookRegistered = false;
    curSeeingHideObject = nullptr;
}


// -----------------------------------------------------------
// [ FSM ]
// -----------------------------------------------------------

void AdultGhostController::InitFSMStates()
{
    //  Patrol, Chase, Search, Return, Attack, None
    fsmStates[(int)AdultGhostState::Patrol] = new AdultGhost_Patrol(this);
    fsmStates[(int)AdultGhostState::Chase] = new AdultGhost_Chase(this);
    fsmStates[(int)AdultGhostState::Search] = new AdultGhost_Search(this);
    fsmStates[(int)AdultGhostState::Return] = new AdultGhost_Return(this);
    fsmStates[(int)AdultGhostState::Attack] = new AdultGhost_Attack(this);
}

void AdultGhostController::ChangeState(AdultGhostState nextState)
{
    if (currentState == fsmStates[(int)nextState])
        return;

    if (currentState)
        currentState->Exit();

    currentState = fsmStates[(int)nextState];
    this->state = nextState;

    if (currentState)
        currentState->Enter();
}


void AdultGhostController::LoadAnimation()
{
    // 애니메이션 파일 로드
    FBXResourceManager::Instance().LoadAnimationByPath(fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\Adult_Ghost\\ani_idle_ghost.fbx", "Idle");
    FBXResourceManager::Instance().LoadAnimationByPath(fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\Adult_Ghost\\ani_attack_ghost.fbx", "Attack", false);
    FBXResourceManager::Instance().LoadAnimationByPath(fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\Adult_Ghost\\ani_attackdelay_ghost.fbx", "AttackDelay", false);

    // 클립 생성
    auto idleClip = animController->FindClip("Idle");
    auto attackClip = animController->FindClip("Attack");
    auto attackDelayClip = animController->FindClip("AttackDelay");

    if (!idleClip || !attackClip || !attackDelayClip)
    {
        cout << "[AdultGhostController] Clip not found!\n" << endl;
        return;
    }
    else
    {
        cout << "[AdultGhostController] Animation Load Success" << endl;
    }

    // 상태 등록
    animController->AddState(std::make_unique<AnimationState>("Idle", idleClip, animController));
    animController->AddState(std::make_unique<AnimationState>("Attack", attackClip, animController));
    animController->AddState(std::make_unique<AnimationState>("AttackDelay", attackDelayClip, animController));
}


// -------------------------------------------------
// Helper
// -------------------------------------------------

void AdultGhostController::ResetAgentForMove(float speed)
{
    if (!agent) return;

    agent->SetSpeed(speed);
    agent->ClearTarget();
}


// Ai가 Target을 보고 있는가? // TODO : FOV, Dist 값 매개변수로 받기 
bool AdultGhostController::IsSeeing(GameObject* target) const
{
    if (!target)
        return false;

    // 플레이어인지 확인
    auto* playerController = target->GetComponent<PlayerController>();
    if (playerController)
    {
        // 플레이어가 Hide 상태면 감지하지 않음
        if (playerController->GetPlayerState() == PlayerState::Hide)
        {
            std::cout << "[AdultGhostController] Player is Hiding, Can't See" << std::endl;
            return false;
        }
    }

    // 시야 체크
    return vision->CheckVision(target, 30, 400);
}

bool AdultGhostController::CanDetectPlayer() const
{
    auto* player = GetAITarget();
    return IsSeeing(player);
}

// 시야에 안 보여도 소실 거리 안이면 계속 추격 가능하게
bool AdultGhostController::CanKeepChase()
{
    auto* player = GetPlayer();
    if (!player) return false;

    auto* playerController = player->GetComponent<PlayerController>();
    if (playerController && playerController->GetPlayerState() == PlayerState::Hide)
        return false;

    Vector3 myPos = GetOwner()->GetTransform()->GetWorldPosition();
    Vector3 playerPos = player->GetTransform()->GetWorldPosition();

    const float loseDistance = 500.0f;
    return Vector3::Distance(myPos, playerPos) <= loseDistance;
}

// Object Getter 
GameObject* AdultGhostController::GetAITarget() const // Raycast 전용 (IsSeeing()에서 사용)
{
    return SceneSystem::Instance().GetCurrentScene() ->GetGameObjectByName("AITarget");
}
GameObject* AdultGhostController::GetPlayer() const // 플레이어 
{
    return SceneSystem::Instance().GetCurrentScene() ->GetGameObjectByName("Player");
}

// Ai가 플레이어의 기척 범위에 들어왔는가? 
bool AdultGhostController::IsPlayerInSenseRange() 
{
    auto* playerObj = GetPlayer();
    if (!playerObj) return false;

    auto* playerController = playerObj->GetComponent<PlayerController>();
    if (!playerController) return false;

    float senseRadius = playerController->GetCurSenseRadiuse();
    if (senseRadius <= 0) return false;

    Vector3 pPos = playerObj->GetTransform()->GetWorldPosition();
    Vector3 gPos = this->GetOwner()->GetTransform()->GetWorldPosition();

    return Vector3::Distance(pPos, gPos) <= senseRadius;
}

void AdultGhostController::StartPostBabyCare()
{
    postCareTimer = 0.0f;
    postCareActive = true;
    if (target)
    {
        // forcedTargetPos = target->GetTransform()->GetLocalPosition(); // 플레이어 위치 저장
        forcedTargetPos = target->GetTransform()->GetWorldPosition();
    }

    // PostBabyCare 동안 기존 target 제거
    target = nullptr;

    if (agent)
        agent->ClearTarget();
}


// -------------------------------------------------
// Interaction
// -------------------------------------------------

// 플레이어에서 호출 
void AdultGhostController::OnPlayerNoise(const Vector3& noiseWorldPos)
{
    // Patrol 상태인 귀신만 반응
    if (state != AdultGhostState::Patrol) return;

    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    int cx, cy;
    if (!grid->WorldToGridFromCenter(noiseWorldPos, cx, cy))
        return;

    // Search 상태로 전환 + 목표 좌표 설정 
    lastPlayerGrid = { cx, cy, true };

    searchReason = SearchReason::FromPatrol;
    ChangeState(AdultGhostState::Search);
}

void AdultGhostController::OnBabyCry(const Vector3& cryWorldPos)
{
    if (state != AdultGhostState::Patrol) return;

    forcedTargetPos = cryWorldPos;
    chaseReason = ChaseReason::FromBabyCry;
    ChangeState(AdultGhostState::Chase);
}

void AdultGhostController::OnAttackHit()
{
    if (state == AdultGhostState::Chase)
    {
        ChangeState(AdultGhostState::Attack);
    }
}

void AdultGhostController::SetAITarget(GameObject* newTarget)
{
    if (!agent) return;

    target = newTarget;
    if (!target) return;

    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    int tx, ty;
    // if (grid->WorldToGridFromCenter(target->GetTransform()->GetWorldPosition(), tx, ty))
    if (grid->WorldToGridFromCenter(target->GetTransform()->GetLocalPosition(), tx, ty))
    {
        agent->SetTarget(tx, ty);
    }
}


// -------------------------------------------------
// WayPoint 관련 
// -------------------------------------------------

void AdultGhostController::SetNextPatrolTarget()
{
    if (patrolPointCount <= 0 || !agent)
        return;

    GridPos& p = patrolPoints[patrolIndex];
    agent->SetTarget(p.x, p.y);

    lastVisitedWaypoint = patrolIndex;

    patrolIndex++;
    if (patrolIndex >= patrolPointCount)
        patrolIndex = 0;

    std::cout << "[Adult Patrol] Next Waypoint: "
        << patrolIndex << " (" << p.x << "," << p.y << ")\n";
}

void AdultGhostController::SetReturnToLastWaypoint()
{
    if (!agent) return;

    if (lastVisitedWaypoint >= 0)
    {
        GridPos& p = patrolPoints[lastVisitedWaypoint];
        agent->SetTarget(p.x, p.y);
        return;
    }

    // 마지막으로 방문한 웨이포인트가 없으면 처음 웨이포인트로 설정
    if (patrolPointCount > 0)
    {
        GridPos& p = patrolPoints[0];
        agent->SetTarget(p.x, p.y);
        return;
    }

    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    int tx, ty;
    if (grid->WorldToGridFromCenter(initialPosition, tx, ty))
        agent->SetTarget(tx, ty);
}
