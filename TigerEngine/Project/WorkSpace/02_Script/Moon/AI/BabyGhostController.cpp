#include "BabyGhostController.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

#include "Object/GameObject.h"
#include "EngineSystem/SceneSystem.h"

#include "FSM/IBabyGhostState.h"
#include "FSM/BabyGhost_Patrol.h"
#include "FSM/BabyGhost_Search.h"
#include "FSM/BabyGhost_Cry.h"
#include "FSM/BabyGhost_Return.h"

#include "../../Woo/Object/HideObject.h"
#include "../../Woo/Player/PlayerController.h"
#include "../../Ho/Sound/EnemySoundSource.h"


REGISTER_COMPONENT(BabyGhostController)

RTTR_REGISTRATION
{
    rttr::registration::class_<BabyGhostController>("BabyGhostController")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

nlohmann::json BabyGhostController::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void BabyGhostController::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

// Util 
float _WrapAngleRad(float a)
{
    while (a > XM_PI)  a -= XM_2PI;
    while (a < -XM_PI) a += XM_2PI;
    return a;
}


// -----------------------------------------------------------
// [ Process ]
// -----------------------------------------------------------

void BabyGhostController::OnStart()
{
    agent = GetOwner()->GetComponent<AgentComponent>();
    vision = GetOwner()->GetComponent<VisionComponent>();
    fbxRenderer = GetOwner()->GetComponent<FBXRenderer>();
    fbxData = GetOwner()->GetComponent<FBXData>();
    animController = GetOwner()->GetComponent<AnimationController>();
    enemySound = GetOwner()->GetComponent<EnemySoundSource>();

    if (!agent || !vision || !fbxRenderer || !fbxData || !animController)
    {
        std::cout << "[BabyGhostController] Component Missing" << std::endl;
        return;
    }

    // Hide Object 모두 수집 
    hideObjects = SceneUtil::GetObjectsByName("HideObject");

    LoadAnimation();  

    // 최초 시작 위치 저장
    initialPosition = GetOwner()->GetTransform()->GetWorldPosition(); // local X

    InitFSMStates();
    ChangeState(BabyGhostState::Patrol);
}

void BabyGhostController::OnUpdate(float delta)
{
    if (currentState)
    {
        currentState->ChangeStateLogic();
        currentState->Update(delta);
    }
}

void BabyGhostController::OnFixedUpdate(float dt)
{
    if (currentState)
    {
        currentState->FixedUpdate(dt);
    }
}

void BabyGhostController::OnDestory()
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

void BabyGhostController::InitFSMStates()
{
    fsmStates[(int)BabyGhostState::Patrol] = new BabyGhost_Patrol(this);
    fsmStates[(int)BabyGhostState::Search] = new BabyGhost_Search(this);
    fsmStates[(int)BabyGhostState::Return] = new BabyGhost_Return(this);
    fsmStates[(int)BabyGhostState::Cry] = new BabyGhost_Cry(this);
}

void BabyGhostController::ChangeState(BabyGhostState nextState)
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


void BabyGhostController::LoadAnimation()
{
    // 애니메이션 파일 로드
    FBXResourceManager::Instance().LoadAnimationByPath(fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\Baby_Ghost\\ani_idle_babyghost.fbx", "Idle");
    FBXResourceManager::Instance().LoadAnimationByPath(fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\Baby_Ghost\\ani_cry_babyghost.fbx", "Cry");

    // 클립 생성
    auto idleClip = animController->FindClip("Idle");
    auto cryClip = animController->FindClip("Cry");

    if (!idleClip || !cryClip)
    {
        cout << "[BabyGhostController] Clip not found!\n" << endl;
        return;
    }
    else
    {
        cout << "[BabyGhostController] Animation Load Success" << endl;
    }

    // 상태 등록
    animController->AddState(std::make_unique<AnimationState>("Idle", idleClip, animController));
    animController->AddState(std::make_unique<AnimationState>("Cry", cryClip, animController));
}

// -------------------------------------------------
// Movement
// -------------------------------------------------
bool BabyGhostController::MoveToTarget(float delta)
{
    if (!agent || !agent->hasTarget) return false;

    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return false;

    // 경로 없으면 생성
    if (agent->path.empty())
    {
        agent->path = grid->FindPath(agent->cx, agent->cy, agent->targetCX, agent->targetCY);
        if (agent->path.empty()) return false;
    }

    // 다음 칸 이동 
    auto next = agent->path.front();
    Vector3 targetPos = grid->GridToWorldFromCenter(next.first, next.second);

    Vector3 pos = agent->GetOwner()->GetTransform()->GetWorldPosition();
    Vector3 dir = targetPos - pos;
    dir.y = 0;

    // 해당 칸 도착 
    if (dir.Length() < agent->reachDist)
    {
        agent->cx = next.first;
        agent->cy = next.second;
        agent->path.erase(agent->path.begin());
        return agent->path.empty(); // 이 칸은 끝까지 가도록 
    }

    dir.Normalize();
    agent->MoveAgent(dir, agent->patrolSpeed, delta);
    RotateByDirection(dir, delta);

    return false;
}

void BabyGhostController::RotateByDirection(const Vector3& moveDir, float delta)
{
    if (moveDir.LengthSquared() <= 0.0001f)
        return;

    auto tr = GetOwner()->GetTransform();

    Vector3 dir = -moveDir; // 모델 반전 보정
    float targetYaw = atan2f(dir.x, dir.z);
    float currentYaw = tr->GetYaw();

    float deltaYaw = _WrapAngleRad(targetYaw - currentYaw);

    float turnSpeed = 6.0f; // 플레이어보다 느리게
    float newYaw = currentYaw + deltaYaw * turnSpeed * delta;

    tr->SetEuler(Vector3(0.f, newYaw, 0.f));
}


// -------------------------------------------------
// Helper
// -------------------------------------------------

void BabyGhostController::ResetAgentForMove(float speed)
{
    agent->patrolSpeed = speed;
    agent->externalControl = true;
    agent->hasTarget = false;
    agent->path.clear();
    agent->isWaiting = false;
}

// Ai가 Target을 보고 있는가? // TODO : FOV, Dist 값 매개변수로 받기 
// TODO : 플레이어가 Hide 상태이면, Target을 못봐야 함
bool BabyGhostController::IsSeeing(GameObject* target) const
{
    return target && vision->CheckVision(target, 30, 400);
}

// Object Getter 
GameObject* BabyGhostController::GetAITarget() const
{
    return SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("AITarget");
}
GameObject* BabyGhostController::GetPlayer() const
{
    return SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Player");
}

// Ai가 플레이어의 기척 범위에 들어왔는가? 
bool BabyGhostController::IsPlayerInSenseRange()
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


// -------------------------------------------------
// Interaction
// -------------------------------------------------

// 플레이어에서 호출 
void BabyGhostController::OnPlayerNoise(const Vector3& noiseWorldPos)
{
    // Patrol 상태인 귀신만 반응
    if (state != BabyGhostState::Patrol) return;

    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    int cx, cy;
    if (!grid->WorldToGridFromCenter(noiseWorldPos, cx, cy))
        return;

    // Search 상태로 전환 + 목표 좌표 설정 
    lastPlayerGrid = { cx, cy, true };

    ChangeState(BabyGhostState::Search);
}
