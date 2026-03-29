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
// Helper
// -------------------------------------------------

void BabyGhostController::ResetAgentForMove(float speed)
{
    if (!agent) return;

    agent->SetSpeed(speed);
    agent->ClearTarget();
}

// Ai가 Target을 보고 있는가? - Hide 상태면 시야 감지하지 않음
bool BabyGhostController::IsSeeing(GameObject* target) const
{
    if (!target)
        return false;

    auto* playerController = target->GetComponent<PlayerController>();
    if (playerController && playerController->GetPlayerState() == PlayerState::Hide)
        return false;

    return vision->CheckVision(target, 30, 400);
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

    if (playerController->GetPlayerState() == PlayerState::Hide)
        return false;

    float senseRadius = playerController->GetCurSenseRadiuse();
    if (senseRadius <= 0) return false;

    Vector3 pPos = playerObj->GetTransform()->GetWorldPosition();
    Vector3 gPos = GetOwner()->GetTransform()->GetWorldPosition();

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
    searchReason = SearchReason_Baby::FromPatrol;
    ChangeState(BabyGhostState::Search);
}


// -------------------------------------------------
// WayPoint 관련 
// -------------------------------------------------

void BabyGhostController::SetNextPatrolTarget()
{
    if (patrolPointCount <= 0 || !agent)
        return;

    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid)
        return;

    int myX, myY;
    if (!grid->WorldToGridFromCenter(GetOwner()->GetTransform()->GetWorldPosition(), myX, myY))
        return;

    // 현재 위치와 같은 waypoint는 건너뛰기
    for (int i = 0; i < patrolPointCount; ++i)
    {
        GridPos_Baby& p = patrolPoints[patrolIndex];

        if (p.x != myX || p.y != myY)
        {
            agent->SetTarget(p.x, p.y);

            std::cout << "[Baby Patrol] Next Waypoint: "
                << patrolIndex << " (" << p.x << "," << p.y << ")\n";

            patrolIndex++;
            if (patrolIndex >= patrolPointCount)
                patrolIndex = 0;

            return;
        }

        patrolIndex++;
        if (patrolIndex >= patrolPointCount)
            patrolIndex = 0;
    }

    // 전부 현재 칸이면 아무 것도 안 함
    // std::cout << "[Baby Patrol] No valid next waypoint found.\n";
}

bool BabyGhostController::IsPlayerHidden() const
{
    auto* playerObj = GetPlayer();
    if (!playerObj) return false;

    auto* playerController = playerObj->GetComponent<PlayerController>();
    if (!playerController) return false;

    return playerController->GetPlayerState() == PlayerState::Hide;
}



// 현재 위치에서 가장 가까운 순찰 포인트로 복귀
void BabyGhostController::SetReturnToNearestPatrolTarget()
{
    if (!agent || patrolPointCount <= 0)
        return;

    auto grid = GridSystem::Instance().GetMainGrid();
    if (!grid) return;

    int myX, myY;
    if (!grid->WorldToGridFromCenter(GetOwner()->GetTransform()->GetWorldPosition(), myX, myY))
        return;

    int bestIndex = 0;
    int bestDist = INT_MAX;

    for (int i = 0; i < patrolPointCount; ++i)
    {
        int dist = abs(patrolPoints[i].x - myX) + abs(patrolPoints[i].y - myY);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestIndex = i;
        }
    }

    patrolIndex = bestIndex; // 다음 Patrol이 자연스럽게 이어지도록
    agent->SetTarget(patrolPoints[bestIndex].x, patrolPoints[bestIndex].y);
}