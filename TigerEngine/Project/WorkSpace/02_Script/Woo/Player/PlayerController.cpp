#include "PlayerController.h"
#include "Util/DebugDraw.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"
#include <Util/PhysXUtils.h>

#include "Object/GameObject.h"
#include "System/InputSystem.h"
#include "EngineSystem/PhysicsSystem.h"
#include "EngineSystem/CameraSystem.h"
#include "RenderPass/ParticleSource/Effect.h"

#include "FSM/IPlayerState.h"
#include "FSM/Player_Idle.h"
#include "FSM/Player_Walk.h"
#include "FSM/Player_Run.h"
#include "FSM/Player_Sit.h"
#include "FSM/Player_SitWalk.h"
#include "FSM/Player_Hide.h"
#include "FSM/Player_Hit.h"
#include "FSM/Player_Cook.h"
#include "FSM/Player_Die.h"

#include "../Object/SearchObject.h"
#include "../Object/HideObject.h"
#include "../Inventory/Inventory.h"
#include "../Item/Item.h"
#include "../Camera/CameraController.h"
#include "../MiniGame/MiniGameManager.h"
#include "../JesaSang/JesaSangManager.h"
#include "../Altar/AltarManager.h"
#include "PlayerItemVisualizer.h"
#include "PlayerThreatMonitor.h"
#include "DialogueUI/DialogueUIController.h"
#include "../Manager/GameManager.h"
#include "../Manager/QuestManager.h"
#include "../UI/MainGameUIManager.h"
#include "../CookingZone/CookingZone.h"
#include "../../Ron/MiniMapTest/MiniMapManager.h"
#include "../../Ho/Sound/PlayerSoundSource.h"
#include "../../Ho/Sound/SoundManager.h"


REGISTER_COMPONENT(PlayerController)

RTTR_REGISTRATION
{
    rttr::registration::class_<PlayerController>("PlayerController")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

/*-------[ Component Process ]-------------------------------------*/
void PlayerController::OnStart()
{
    // get components
    transform = GetOwner()->GetComponent<Transform>();
    fbxRenderer = GetOwner()->GetComponent<FBXRenderer>();
    fbxData = GetOwner()->GetComponent<FBXData>();
    animController = GetOwner()->GetComponent<AnimationController>();
    sound = GetOwner()->GetComponent<PlayerSoundSource>();
    fireEffect = GetOwner()->GetChildByName("Player_FireEffect")->GetOwner()->GetComponent<Effect>();
    hitEffect = GetOwner()->GetChildByName("Player_HitEffect")->GetOwner()->GetComponent<AnimationController>();

    cct = GetOwner()->GetComponent<CharacterControllerComponent>();
    inventory = GetOwner()->GetComponent<Inventory>();
    visualizer = GetOwner()->GetComponent<PlayerItemVisualizer>();
    threatMonitor = GetOwner()->GetComponent<PlayerThreatMonitor>();
    dialogueController = GetOwner()->GetComponent<DialogueUIController>();
    
    camController = CameraSystem::Instance().GetCurrCamera()->GetOwner()->GetComponent<CameraController>();

    // debug
    if (!fbxRenderer || !cct || !inventory || !camController || !fbxData || !sound ||
        !animController || !dialogueController || !fireEffect || !hitEffect ||
        !threatMonitor || !visualizer)
    {
        cout << "[Player] Missing COmponet!" << endl;
    }

    // load animation
    LoadAnimation();

    // init fsm
    InitFSMStates();
    ChangeState(PlayerState::Idle);

    // init stat
    InitStat();
}

void PlayerController::OnUpdate(float delta)
{
    // input
    InputProcess();
    
    // fsm
    if(curState)
    {
        curState->ChangeStateLogic();
        curState->Update(delta);
    }

    // interaction cheak
    InteractionCheak(delta);

    // hit duration
    UpsateHitDuration(delta);
}

void PlayerController::OnFixedUpdate(float delta)
{
    // fsm
    if (curState)
    {
        curState->FixedUpdate(delta);
    }

    // movement
    Move(delta);
    Rotation(delta);
}

void PlayerController::OnDestory()
{

}


/*-------[ Collision Event ]-------------------------------------*/
void PlayerController::OnTriggerEnter(PhysicsComponent*)
{
}

void PlayerController::OnTriggerStay(PhysicsComponent*)
{
}

void PlayerController::OnTriggerExit(PhysicsComponent*)
{
}

void PlayerController::OnCollisionEnter(PhysicsComponent*)
{
}

void PlayerController::OnCollisionStay(PhysicsComponent*)
{
}

void PlayerController::OnCollisionExit(PhysicsComponent*)
{
}


/*-------[ JSON ]-------------------------------------*/
nlohmann::json PlayerController::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void PlayerController::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

/*----------------------------------------------------------------*/
/*-------[ FSM ]--------------------------------------------------*/
/*----------------------------------------------------------------*/
void PlayerController::InitFSMStates()
{
    fsmStates[(int)PlayerState::Idle] = new Player_Idle(this);
    fsmStates[(int)PlayerState::Walk] = new Player_Walk(this);
    fsmStates[(int)PlayerState::Run] = new Player_Run(this);
    fsmStates[(int)PlayerState::Sit] = new Player_Sit(this);
    fsmStates[(int)PlayerState::SitWalk] = new Player_SitWalk(this);
    fsmStates[(int)PlayerState::Hide] = new Player_Hide(this);
    fsmStates[(int)PlayerState::Hit] = new Player_Hit(this);
    fsmStates[(int)PlayerState::Cook] = new Player_Cook(this);
    fsmStates[(int)PlayerState::Die] = new Player_Die(this);
}

void PlayerController::ChangeState(PlayerState nextState)
{
    if(curState == fsmStates[(int)nextState])
        return;

    if(curState)
        curState->Exit();

    curState = fsmStates[(int)nextState];
    this->state = nextState;

    if(curState)
        curState->Enter();
}


/*-------[ Animation Create ]-------------------------------------*/
void PlayerController::LoadAnimation()
{
    // 애니메이션 파일 로드
    FBXResourceManager::Instance().LoadAnimationByPath(fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\FuckingAssimp\\ani_idle_character.fbx", "Idle");
    FBXResourceManager::Instance().LoadAnimationByPath(fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\FuckingAssimp\\ani_walk_character.fbx", "Walk");
    FBXResourceManager::Instance().LoadAnimationByPath(fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\FuckingAssimp\\ani_run_character.fbx", "Run");
    FBXResourceManager::Instance().LoadAnimationByPath(fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\FuckingAssimp\\ani_sit_character.fbx", "Sit");
    FBXResourceManager::Instance().LoadAnimationByPath(fbxData->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\FuckingAssimp\\ani_cry_character.fbx", "Hit");

    // 클립 생성
    auto idleClip = animController->FindClip("Idle");
    auto walkClip = animController->FindClip("Walk");
    auto runClip = animController->FindClip("Run");
    auto sitClip = animController->FindClip("Sit");
    auto hitClip = animController->FindClip("Hit");

    if (!idleClip  || !walkClip || !runClip || !sitClip || !hitClip)
    {
        cout << "[Player Animation] Clip not found!\n" << endl;
        return;
    }

    // 상태 등록
    animController->AddState(std::make_unique<AnimationState>("Idle", idleClip, animController));
    animController->AddState(std::make_unique<AnimationState>("Walk", walkClip, animController));
    animController->AddState(std::make_unique<AnimationState>("Run", runClip, animController));
    animController->AddState(std::make_unique<AnimationState>("Sit", sitClip, animController));
    animController->AddState(std::make_unique<AnimationState>("Hit", hitClip, animController));


    // Effect Animatinon
    FBXResourceManager::Instance().LoadAnimationByPath(hitEffect->GetOwner()->GetComponent<FBXData>()->GetFBXInfo(), 
        "..\\Assets\\Resource\\Effect\\ani_confused_mark.fbx", "HitEffect");
    auto effectHitClip = hitEffect->FindClip("HitEffect");
    hitEffect->AddState(std::make_unique<AnimationState>("HitEffect", effectHitClip, hitEffect));
    hitEffect->ChangeState("HitEffect");
    
    if (!effectHitClip)
    {
        cout << "[Player Effect Animation] Clip not found!\n" << endl;
        return;
    }
}

/*-------[ Init ]-------------------------------------*/
void PlayerController::InitStat()
{
    curLife = life;
}

/*-------[ Input ]-------------------------------------*/
void PlayerController::InputProcess()
{
    if (isInputLocked) // 선민 | 02.11 
    {
        isMoveLKey = false;
        isMoveRKey = false;
        isMoveFKey = false;
        isMoveBKey = false;
        isSitKey = false;
        isRunKey = false;
        isInteractionKey = false;
        return;
    }

    // key
    isMoveLKey = Input::GetKey(moveL_Key);
    isMoveRKey = Input::GetKey(moveR_Key);
    isMoveFKey = Input::GetKey(moveF_Key);
    isMoveBKey = Input::GetKey(moveB_Key);
    isSitKey = Input::GetKey(sit_Key);
    isRunKey = Input::GetKey(run_Key);
    isInteractionKey = Input::GetKey(interaction_Key);


    // ------ Debuging ----------
    // cheet
    if (Input::GetKeyDown(cheatKey))
        isCheatMode = !isCheatMode;

    if (isCheatMode)
    {
        // game success
        if (Input::GetKeyDown(Keyboard::D1))
            GameManager::Instance()->GameSuccess();

        // game fail
        if (Input::GetKeyDown(Keyboard::D2))
            GameManager::Instance()->GameOver();

        // ai attack test
        if (Input::GetKeyDown(Keyboard::D3))
            TakeAttack();

        // 제단 연출
        //if (Input::GetKeyDown(Keyboard::D4))
            

        // quarter view
        if (Input::GetKeyDown(Keyboard::D5))
            camController->SetViewMode(CameraController::ViewMode::Quarter);

        // top view
        if (Input::GetKeyDown(Keyboard::D6))
            camController->SetViewMode(CameraController::ViewMode::Top);

        // front view
        if (Input::GetKeyDown(Keyboard::D7))
            camController->SetViewMode(CameraController::ViewMode::Front);
    }
}

/*-------[ Movement ]----------------------------------*/
void PlayerController::Move(float delta)
{
    if (!cct) return;

    //// 연출 강제 이동 // 선민 | 02.11 
    //if (isForcedMove)
    //{
    //    cct->MovePlayer(forcedDir, forcedSpeed, delta);
    //    return;
    //}

    if (isInputLocked) return; // 선민 | 02.11 

    // cct->m_MoveSpeed = curSpeed;
    cct->MovePlayer(lookDir, curSpeed, delta);
    // cct->MoveCharacter(lookDir, delta);
}

static float WrapAngleRad(float a)      // util
{
    while (a > DirectX::XM_PI)  a -= DirectX::XM_2PI;
    while (a < -DirectX::XM_PI) a += DirectX::XM_2PI;
    return a;
}

void PlayerController::Rotation(float delta)
{
    // if (isForcedMove) return;   // 선민 | 02.11 
    if (isInputLocked) return;  // 선민 | 02.11 
    if (lookDir.LengthSquared() <= 0.0001f)
        return;

    Vector3 rotationDir = -lookDir;         // 너 왜 반전이닝?

    float targetYaw = atan2f(rotationDir.x, rotationDir.z);
    float currentYaw = transform->GetYaw();
    float deltaYaw = WrapAngleRad(targetYaw - currentYaw);

    float turnSpeed = 10.0f;
    float newYaw = currentYaw + deltaYaw * turnSpeed * delta;

    transform->SetEuler(Vector3(0.0f, newYaw, 0.0f));
}

/*-------[ Interaction ]----------------------------------*/
void PlayerController::InteractionCheak(float delta)
{
    SerachObjectInteraction(delta);     // 수색 오브젝트 수색 Interaction
    HideObjectInteraction(delta);       // 은신 오브젝트 은신 Interaction
    CookingInteraction(delta);          // 조리대 미니게임 시작 Interaction
    PutFoodJesaSangInteraction(delta);  // 제사상 음식 올리기 Interaction
    GetItemAltarInteraction(delta);     // 제단에서 아이템 가져오기 Interaction
}

void PlayerController::SerachObjectInteraction(float dt)
{
    // interaction x -> reset
    if (!isInteractionKey || !isPossibleSearch || curSerachObject == nullptr)
    {
        searchTimer = 0.0f;
        return;
    }

    // 최초 인터랙션 기믹 설명
    if (!isExplainedSearchObject && Input::GetKeyDown(interaction_Key))
    {
        dialogueController->ShowInteractionHintAndPause(L"this object is Search Object!");
        isExplainedSearchObject = true;
    }

    // inventory full
    if (inventory->HasItem())
    {
        cout << "[Player] Inventory Full! Can't interaction" << endl;
        return;
    }

    // sound
    if(Input::GetKeyDown(interaction_Key))
        SoundManager::Instance()->PlaySFX(SFXType::FindObj_Interaction_2_Sound);

    // holding
    searchTimer += dt;
    float progress = searchTimer / searchTime;
    if (progress > 1.0f) progress = 1.0f;

    // UI - interaction
    curSerachObject->UIGaugeUpate(progress);

    // search object interaction
    if (searchTimer >= searchTime)
    {
        // search object interaction
        std::unique_ptr<IItem> item = curSerachObject->Interaction();

        // item get or fail
        if (item)
        {
            // 미니맵 연결
            if (item->itemType == ItemType::Piece)
            {
                // 미니맵
                auto* ob = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("UI_MiniMap_Controller");
                auto* minimap = ob->GetComponent<MiniMapManager>();
                if(item->itemId == "0")
                    minimap->TriggerPieceCollected(0);
                else if (item->itemId == "1")
                    minimap->TriggerPieceCollected(1);
                else if (item->itemId == "2")
                    minimap->TriggerPieceCollected(2);
                else if (item->itemId == "3")
                    minimap->TriggerPieceCollected(3);
                else if (item->itemId == "4")
                    minimap->TriggerPieceCollected(4);

                // sound
                SoundManager::Instance()->PlaySFX(SFXType::FindObj_Acquiremap_Sound);
            }
            // 퀘스트 1 : [탐색] 제사준비 : 최조로 음식 재료 획득시 달성
            else if (item->itemType == ItemType::Ingredient)
            {
                QuestManager::Instance()->StepComplete(1);

                // sound
                SoundManager::Instance()->PlaySFX(SFXType::FindObj_Acquireitem_Sound);
            }

            // item get
            visualizer->VisualOnItem(item->itemId);
            inventory->AddItem(std::move(item));
        }
        else
            std::cout << "[Player] Fail Get Item... " << std::endl;

        // clear
        isPossibleSearch = false;
        curSerachObject = nullptr;
        searchTimer = 0.0f;
    }
}

void PlayerController::HideObjectInteraction(float dt)
{
    // interaction x
    if (!isPossibleHide || !curHideObject)
        return;

    // hit, die 상태라면 return (맞겠지?) -> hit일떄 은신된대.
    if (state == PlayerState::Die)
        return;

    // 최초 인터랙션 기믹 설명
    if (!isExplainedHideObject && Input::GetKeyDown(interaction_Key))
    {
        dialogueController->ShowInteractionHintAndPause(L"this object is Hide Object!");
        isExplainedHideObject = true;
    }

    // hide
    if (Input::GetKeyDown(interaction_Key) && curHideObject->IsPossibleHide())
    {
        // hit->hide 일 경우 hide가 끝나면 hit으로 복귀하도록 플래그 설정
        resumeHitAfterHide = (state == PlayerState::Hit);

        // hide
        ChangeState(PlayerState::Hide);
        curHideObject->StartHide(this);
    }
}

void PlayerController::CookingInteraction(float dt)
{
    // interaction x -> reset
    if (!isInteractionKey || !isPossibleCooking || !inventory->HasItem())
    {
        cookInteractionTimer = 0.0f;
        return;
    }

    // ingredient
    if (inventory->GetCurItemType() != ItemType::Ingredient)
    {
        cookInteractionTimer = 0.0f;
        return;
    }

    // 최초 인터랙션 기믹 설명
    if(!isExplainedCookingZone)
    {
        dialogueController->ShowInteractionHintAndPause(L"여기서 음식을 만들 수 있겠어. 망치지 않게 집중해야지!");
        isExplainedCookingZone = true;
    }

    // holding
    cookInteractionTimer += dt;
    float progress = cookInteractionTimer / cookInteractionTime;
    if (progress > 1.0f) progress = 1.0f;

    // UI - interaction
    CookingZone::Instance()->UIGaugeUpate(progress);

    // cooking interaction
    if (cookInteractionTimer >= cookInteractionTime)
    {
        // mini game start
        visualizer->VisualOffItem();
        MiniGameManager::Instance()->StartMiniGame(std::move(inventory->TakeCurItem()));
        ChangeState(PlayerState::Cook);
        cookInteractionTimer = 0.0f;

        // ui clear
        CookingZone::Instance()->UIGaugeUpate(0.0);
    }
}

void PlayerController::PutFoodJesaSangInteraction(float dt)
{
    // interaction x -> reset
    if (!isInteractionKey || !isPossiblePutFood ||
        !inventory->HasItem() || inventory->GetCurItemType() != ItemType::Food)
    {
        putFoodTimer = 0.0f;
        return;
    }

    // holding
    putFoodTimer += dt;
    float progress = putFoodTimer / putFoodTime;
    if (progress > 1.0f) progress = 1.0f;

    // UI - interaction
    JesaSangManager::Instance()->UIGaugeUpate(progress);

    // 제사상에 음식 올리기 interaction
    if (putFoodTimer >= putFoodTime)
    {
        std::unique_ptr<IItem> food = inventory->TakeCurItem();
        JesaSangManager::Instance()->ReceiveFood(std::move(food));
        visualizer->VisualOffItem();
        visualizer->VisualItemIDNullSet();

        // 퀘스트 3 : [운반] 차려지는 상 : 최조로 제사상에 음식을 올렸을시 달성
        QuestManager::Instance()->StepComplete(3);

        // sound
        SoundManager::Instance()->PlaySFX(SFXType::GoalObj_Sound);

        // clear
        putFoodTimer = 0.0f;
    }
}

void PlayerController::GetItemAltarInteraction(float dt)
{
    // interaction x -> reset
    if (!isInteractionKey || !isPossibleGetFood || !AltarManager::Instance()->HasItem())
    {
        getItemAltarTimer = 0.0f;
        return;
    }

    // inventory full
    if (inventory->HasItem())
    {
        cout << "[Player] Inventory Full! Can't interaction" << endl;
        return;
    }

    // holding
    getItemAltarTimer += dt;
    float progress = getItemAltarTimer / getItemAltarTime;
    if (progress > 1.0f) progress = 1.0f;

    // UI - interaction
    AltarManager::Instance()->UIGaugeUpate(progress);

    // 제단 아이엠(재료/음식) 가져오기 interaction
    if (getItemAltarTimer >= getItemAltarTime)
    {
        std::unique_ptr<IItem> item = AltarManager::Instance()->GetItem();

        // Dialogue
        if (item->itemType == ItemType::Ingredient)
            dialogueController->ShowDialogueText(L"재료를 되찾았어. 어서 요리를 하러 가야겠어");
        if (item->itemType == ItemType::Food)
            dialogueController->ShowDialogueText(L"완성된 음식을 무사히 되찾았어. 제사상으로 가져가자.");

        visualizer->VisualOnItem(item->itemId);
        inventory->AddItem(std::move(item));

        // clear
        getItemAltarTimer = 0.0f;
    }
}

void PlayerController::UpsateHitDuration(float dt)
{
    // Hit일 땐 항상 업데이트
    if (state == PlayerState::Hit)
    {
        hitTimer += dt;
        invincibleTimer += dt;
        renderDirectorTimer += dt;
    }

    // Hide일 땐 Hit에서 들어온 경우만 업데이트
    else if (state == PlayerState::Hide)
    {
        if (!resumeHitAfterHide) return;

        hitTimer += dt;
        invincibleTimer += dt;
        renderDirectorTimer += dt;
    }
    else
        return;

    if (isPlayerInvincible && invincibleTimer >= hitInvincibleTime)
    {
        isPlayerInvincible = false;
        std::cout << "[Player] Hit Invincible Time End." << std::endl;
    }
}
    

PlayerState PlayerController::GetPlayerState()
{
    return state;
}

/*----------- 외부 호출 Funcs.. -------------------------*/
void PlayerController::SetCurSearchObject(SearchObject* object)
{
    // interaction zone에서 search object를 넘겨줌
    if (object)
    {
        isPossibleSearch = true;
        curSerachObject = object;
    }
    else
    {
        isPossibleSearch = false;
        curSerachObject = nullptr;
    }
}

void PlayerController::SetCurHideObject(HideObject* object)
{
    // interaction zone에서 hide object를 넘겨줌
    if (object)
    {
        isPossibleHide = true;
        curHideObject = object;
    }
    else
    {
        isPossibleHide = false;
        curHideObject = nullptr;
    }
}

void PlayerController::ReceiveMiniGameResult(unique_ptr<IItem> ingredient, bool isSuccess)
{
    if (isSuccess)
    {
        string ingreID = ingredient->itemId;

        // 성공시 음식 생성
        std::unique_ptr<IItem> food;
        if (ingreID == "Ingredient_Apple") food = make_unique<Food>("Apple");
        else if (ingreID == "Ingredient_Pear") food = make_unique<Food>("Pear");
        else if (ingreID == "Ingredient_Batter") food = make_unique<Food>("Batter");
        else if (ingreID == "Ingredient_Tofu") food = make_unique<Food>("Tofu");
        else if (ingreID == "Ingredient_Sanjeok") food = make_unique<Food>("Sanjeok");
        else if (ingreID == "Ingredient_Donggeurangttaeng") food = make_unique<Food>("Donggeurangttaeng");
        else cout << "NO!!!!!!!!!!!!!!!! ingredent id isanham." << endl;

        cout << "[Player] Cooking Success! Get Food : " << food->itemId << endl;

        // 재료 사용
        ingredient.reset();

        // 인벤토리에 완성된 음식 추가
        visualizer->VisualOnItem(food->itemId);
        inventory->AddItem(std::move(food));

        // 퀘스트 2 : [조리] 정성을 담아 : 최초로 미니게임 성공시 달성
        QuestManager::Instance()->StepComplete(2);
    }
    else
    {
        cout << "[Player] Cooking Fail.. Get Ingredient : " << ingredient->itemId << endl;

        // 실패시 재료 다시 돌려받음
        visualizer->VisualOnItem(ingredient->itemId);
        inventory->AddItem(std::move(ingredient));

        // 소음, AI 트리거 발생
        CookingZone::Instance()->StartTriggerWave();
    }
}

void PlayerController::ReceiveMiniGameItem(unique_ptr<IItem> ingredient)
{
    // ESC로 미니게임 강제종료시 재료반 다시 돌려받음
    visualizer->VisualOnItem(ingredient->itemId);
    inventory->AddItem(std::move(ingredient));
}

void PlayerController::TakeAttack()
{
    // cheet
    if (isCheatMode) return;
    
    // 이미 죽은상태 return
    if (state == PlayerState::Die) return;

    // 무적상태 return
    if (isPlayerInvincible)
    {
        cout << "[Player] TakeAttack Fail! isPlayerInvincible" << endl;
        return;
    }
    
    cout << "[Player] Take Damage! " << endl;

    // 아이템이 있다면 제단에 올라감
    if (inventory->HasItem())
    {
        std::unique_ptr<IItem> item = inventory->TakeCurItem();
        visualizer->VisualOffItem();
        visualizer->VisualItemIDNullSet();
        AltarManager::Instance()->ReceiveItem(std::move(item));
        fireEffect->Play();
        cout << "[Player] Drop Item... " << endl;
    }

    // life
    curLife--;
    MainGameUIManager::Instance()->UpdateLifeUI(curLife);

    // Die
    if (curLife <= 0)
    {
        curLife = 0;
        ChangeState(PlayerState::Die);
        GameManager::Instance()->GameOver();

        return;
    }

    // Hit (패닉)
    if(state != PlayerState::Hit)
    {
        hitTimer = 0.0f;
        invincibleTimer = 0.0f;
        renderDirectorTimer = 0.0f;
        isPlayerInvincible = true;

        ChangeState(PlayerState::Hit);
    }
    else
    {
        // 이미 패닉상태였을경우 재시작 (무적상태는 위에서 return)
        hitTimer = 0.0f;
        invincibleTimer = 0.0f;
        renderDirectorTimer = 0.0f;
        isPlayerInvincible = true;

        ChangeState(PlayerState::Idle);
        ChangeState(PlayerState::Hit);
    }
}

float PlayerController::GetCurSenseRadiuse() const
{
    return curSenseRadius;
}

void PlayerController::GameSuceesPlayerStop()
{
    isInputLocked = true;
    fbxRenderer->SetActive(false);
    ChangeState(PlayerState::Idle);
}


//// 선민 | 02.12 ------------------------------------------
//
//void PlayerController::SetForcedMove(const Vector3& dir, float speed)
//{
//    forcedDir = dir;
//    forcedSpeed = speed;
//    isForcedMove = true;
//}
//
//void PlayerController::ClearForcedMove()
//{
//    isForcedMove = false;
//}

