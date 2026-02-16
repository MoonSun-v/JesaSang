#include "TutorialController.h"

#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

#include "System/InputSystem.h"

#include "EngineSystem/SceneSystem.h"
#include "../../Woo/Camera/CameraController.h"


#include "Step/ITutorialStep.h"
#include "Step/TutorialStep_Step1.h"
#include "Step/TutorialStep_Step2.h"
#include "Step/TutorialStep_Step3.h"
#include "Step/TutorialStep_Step4.h"
#include "Step/TutorialStep_Step5.h"
#include "Step/TutorialStep_Step6.h"
#include "Step/TutorialStep_Step7.h"
#include "Step/TutorialStep_Step8.h"
#include "Step/TutorialStep_Step9.h"
#include "../../Woo/Player/PlayerController.h"


REGISTER_COMPONENT(TutorialController)

RTTR_REGISTRATION
{
    rttr::registration::class_<TutorialController>("TutorialController")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

nlohmann::json TutorialController::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void TutorialController::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}


// --------------------------------------------------------


void TutorialController::OnStart() 
{
    // -------------------------------------------------
    // 튜토리얼 세팅
    // -------------------------------------------------
    InitFSMSteps();

    // Step1 연출 
    Blink_Top = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Blink_Top"); 
    Blink_Bottom = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Blink_Bottom"); 

    auto camOb = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("MainCamera");
    if (camOb) camController = camOb->GetComponent<CameraController>();


    // -------------------------------------------------
    // 플레이어 세팅
    // -------------------------------------------------
    player_Obj = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Player");
    player_Obj->GetComponent<PlayerController>()->SetInputLock(true);
    InitPlayerPosition = player_Obj->GetTransform()->GetWorldPosition();



    // -------------------------------------------------
    // 아기 귀신 세팅
    // -------------------------------------------------
    babyGhost_Obj = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Ghost_Baby_Tutorial");
    BabyGhost_animController = babyGhost_Obj->GetComponent<AnimationController>();
    BabyGhost_fbxRenderer = babyGhost_Obj->GetComponent<FBXRenderer>();
    BabyGhost_fbxData = babyGhost_Obj->GetComponent<FBXData>();

    if (!BabyGhost_animController || !BabyGhost_fbxRenderer || !BabyGhost_fbxData)
    {
        std::cout << "[TutorialController] Ghost_Baby_Tutorial Component Missing" << std::endl;
        return;
    }

    LoadBabyGhostAnimation();


    // -------------------------------------------------
    // 어른 귀신 세팅
    // -------------------------------------------------
    adultGhost_Obj = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Ghost_Adult_Tutorial");
    AdultGhost_animController = adultGhost_Obj->GetComponent<AnimationController>();
    AdultGhost_fbxRenderer = adultGhost_Obj->GetComponent<FBXRenderer>();
    AdultGhost_fbxData = adultGhost_Obj->GetComponent<FBXData>();

    if (!AdultGhost_animController || !AdultGhost_fbxRenderer || !AdultGhost_fbxData)
    {
        std::cout << "[TutorialController] Ghost_Adult_Tutorial Component Missing" << std::endl;
        return;
    }

    LoadAdultGhostAnimation();
    InitAdultPosition = adultGhost_Obj->GetTransform()->GetWorldPosition();


    // -------------------------------------------------
    // 튜토리얼 
    // -------------------------------------------------
    ChangeStep(TutorialStep::Step1);
}


// IsComplete() 가 true가 되면 -> 자동으로 Step 전환 
void TutorialController::OnUpdate(float dt)
{
    if (!currentStep) return;

    if (currentStep->IsComplete())
    {
        ChangeStep((TutorialStep)((int)state + 1));
        return;
    }

    currentStep->Update(dt);
}


// -----------------------------------------------------------
// [ Tutorial FSM ]
// -----------------------------------------------------------

void TutorialController::InitFSMSteps()
{
    fsmSteps[(int)TutorialStep::Step1] = new TutorialStep_Step1(this);
    fsmSteps[(int)TutorialStep::Step2] = new TutorialStep_Step2(this);
    fsmSteps[(int)TutorialStep::Step3] = new TutorialStep_Step3(this);
    fsmSteps[(int)TutorialStep::Step4] = new TutorialStep_Step4(this);
    fsmSteps[(int)TutorialStep::Step5] = new TutorialStep_Step5(this);
    fsmSteps[(int)TutorialStep::Step6] = new TutorialStep_Step6(this);
    fsmSteps[(int)TutorialStep::Step7] = new TutorialStep_Step7(this);
    fsmSteps[(int)TutorialStep::Step8] = new TutorialStep_Step8(this);
    fsmSteps[(int)TutorialStep::Step9] = new TutorialStep_Step9(this);
}

void TutorialController::ChangeStep(TutorialStep next)
{
    // None은 튜토리얼 종료
    if (next == TutorialStep::None)
    {
        if (currentStep)
        {
            currentStep->Exit();
            currentStep = nullptr;
        }

        state = TutorialStep::None;

        auto s = SceneSystem::Instance().GetCurrentScene();
        s->LoadToJson("..\\Assets\\Scenes\\04_MainGameScene.json");

        std::cout << "=== Tutorial FINISH ===" << std::endl;
        return;
    }

    if (currentStep == fsmSteps[(int)next]) return;

    if (currentStep) currentStep->Exit();
    currentStep = fsmSteps[(int)next];
    state = next;

    if (currentStep) currentStep->Enter();
}



// -----------------------------------------------------------
// [ 플레이어 ]
// -----------------------------------------------------------

//void TutorialController::LoadPlayerAnimation()
//{
//    // 애니메이션 파일 로드
//    FBXResourceManager::Instance().LoadAnimationByPath(Player_fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\FuckingAssimp\\ani_idle_character.fbx", "Idle");
//    FBXResourceManager::Instance().LoadAnimationByPath(Player_fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\FuckingAssimp\\ani_walk_character.fbx", "Walk");
//    FBXResourceManager::Instance().LoadAnimationByPath(Player_fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\FuckingAssimp\\ani_run_character.fbx", "Run");
//    FBXResourceManager::Instance().LoadAnimationByPath(Player_fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\FuckingAssimp\\ani_sit_character.fbx", "Sit");
//    FBXResourceManager::Instance().LoadAnimationByPath(Player_fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\FuckingAssimp\\ani_cry_character.fbx", "Hit");
//
//    // 클립 생성
//    auto idleClip = Player_animController->FindClip("Idle");
//    auto walkClip = Player_animController->FindClip("Walk");
//    auto runClip = Player_animController->FindClip("Run");
//    auto sitClip = Player_animController->FindClip("Sit");
//    auto hitClip = Player_animController->FindClip("Hit");
//
//    if (!idleClip || !walkClip || !runClip || !sitClip || !hitClip)
//    {
//        cout << "[TutorialController] Clip not found!\n" << endl;
//        return;
//    }
//
//    // 상태 등록
//    Player_animController->AddState(std::make_unique<AnimationState>("Idle", idleClip, Player_animController));
//    Player_animController->AddState(std::make_unique<AnimationState>("Walk", walkClip, Player_animController));
//    Player_animController->AddState(std::make_unique<AnimationState>("Run", runClip, Player_animController));
//    Player_animController->AddState(std::make_unique<AnimationState>("Sit", sitClip, Player_animController));
//    Player_animController->AddState(std::make_unique<AnimationState>("Hit", hitClip, Player_animController));
//
//    // 시작 상태
//    Player_animController->ChangeState("Idle");
//}


// -----------------------------------------------------------
// [ 아기 귀신 ]
// -----------------------------------------------------------

void TutorialController::LoadBabyGhostAnimation()
{
    // 애니메이션 파일 로드
    FBXResourceManager::Instance().LoadAnimationByPath(BabyGhost_fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\Baby_Ghost\\ani_idle_babyghost.fbx", "Idle");
    FBXResourceManager::Instance().LoadAnimationByPath(BabyGhost_fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\Baby_Ghost\\ani_cry_babyghost.fbx", "Cry");

    // 클립 생성
    auto idleClip = BabyGhost_animController->FindClip("Idle");
    auto cryClip = BabyGhost_animController->FindClip("Cry");

    if (!idleClip || !cryClip)
    {
        cout << "[TutorialController] Clip not found!\n" << endl;
        return;
    }

    // 상태 등록
    BabyGhost_animController->AddState(std::make_unique<AnimationState>("Idle", idleClip, BabyGhost_animController));
    BabyGhost_animController->AddState(std::make_unique<AnimationState>("Cry", cryClip, BabyGhost_animController));

    // 시작 상태
    BabyGhost_animController->ChangeState("Idle");
}


// -----------------------------------------------------------
// [ 어른 귀신 ]
// -----------------------------------------------------------

void TutorialController::LoadAdultGhostAnimation()
{
    // 애니메이션 파일 로드
    FBXResourceManager::Instance().LoadAnimationByPath(AdultGhost_fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\Adult_Ghost\\ani_idle_ghost.fbx", "Idle");
    FBXResourceManager::Instance().LoadAnimationByPath(AdultGhost_fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\Adult_Ghost\\ani_attack_ghost.fbx", "Attack", false);

    // 클립 생성
    auto idleClip = AdultGhost_animController->FindClip("Idle");
    auto attackClip = AdultGhost_animController->FindClip("Attack");

    if (!idleClip || !attackClip)
    {
        cout << "[TutorialController] Clip not found!\n" << endl;
        return;
    }

    // 상태 등록
    AdultGhost_animController->AddState(std::make_unique<AnimationState>("Idle", idleClip, AdultGhost_animController));
    AdultGhost_animController->AddState(std::make_unique<AnimationState>("Attack", attackClip, AdultGhost_animController));


    // 시작 상태
    AdultGhost_animController->ChangeState("Idle");
}


// -----------------------------------------------------------
// [ Util ]
// -----------------------------------------------------------
