#pragma once
#include "Components/ScriptComponent.h"
#include "Components/FBXRenderer.h"
#include "Components/FBXData.h"
#include "Components/AnimationController.h"
#include "Components/CharacterControllerComponent.h"

#include <directxtk/Keyboard.h>

class IPlayerState;
class InteractionZone;
class InteractionSensor;
class SearchObject;
class HideObject;
class Inventory;
class CameraController;
class MiniGameManager;
class JesaSangManager;
class PlayerItemVisualizer;
class PlayerThreatMonitor;
class DialogueUIController;
class IItem;
class Effect;
class PlayerSoundSource;

// Player State Enum
enum class PlayerState
{
    Idle, Walk, Run, Sit, SitWalk, Hide, Hit, Cook, Die, None
};

/*
     [ Player Controller Script Component ]

     1) FSM
     2) Input
     3) Movement
     4) Interaction
     5) TODO..
*/
class PlayerController : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    bool isInputLocked = false;  // 선민 Tutorial | 02.11 
public:
    void SetInputLock(bool lock) { isInputLocked = lock; }
    bool IsInputLocked() const { return isInputLocked; }


private:
    // 치트
    Keyboard::Keys cheatKey = Keyboard::D0;;
    bool isCheatMode = false;

private:
    
    // --- [ Component ] ---------------------------
    Transform* transform = nullptr;
    FBXRenderer* fbxRenderer = nullptr;
    FBXData* fbxData = nullptr;
    AnimationController* animController = nullptr;
    CharacterControllerComponent* cct = nullptr;
    PlayerSoundSource* sound = nullptr;

    Effect* fireEffect = nullptr;
    AnimationController* hitEffect = nullptr;

    Inventory* inventory = nullptr;
    PlayerItemVisualizer* visualizer = nullptr;
    PlayerThreatMonitor* threatMonitor = nullptr;
    DialogueUIController* dialogueController = nullptr;

    CameraController* camController = nullptr;

    // --- [ State ] ---------------------------
    PlayerState state = PlayerState::None;
    IPlayerState* curState;
    IPlayerState* fsmStates[9];

    // --- [ Stat (Data) ] --------------------------------
    // life
    int life = 5;

    // speed
    float walkSpeed = 2.0f; 
    float runSpeed  = 3.7f; 
    float sitSpeed = 1.0f;  
    float hitSpeedUpRate = 1.5f;        // walkSpeed * 2.0f

    // sense
    float idleSenseRadius = 0.0f;
    float walkSenseRadius = 200.0f;
    float sitSenseRadius = 0.0f;
    float runSenseRadius = 500.0f;

    float ingreSenseRadius = 100.0f;
    float foodSenseRadius = 200.0f;

    // hit
    float hitDuration = 5.0f;           // 패닉 유지시간
    float hitInvincibleTime = 3.0f;     // 패닉 무적타임
    float renderDirectorTime = 0.2f;    // 패닉 렌더 연출시간


    // --- [ Controll ] ----------------------------
    // cur stat, state
    int curLife;
    float curSpeed = 0.0f;
    Vector3 lookDir = Vector3::Zero;
    float curSenseRadius = 0.0f;         // 현재 기척 범위

    // hit
    bool isPlayerInvincible = false;     // 현재 무적상태 여부
    bool resumeHitAfterHide = false;     // hit->hide hit 초기화 방지
    float hitTimer = 0.0f;               // hit 상태 지속시간 타이머
    float invincibleTimer = 0.0f;        // hit 무적상태 지속시간 타이머
    float renderDirectorTimer = 0.0f;    // 플레이어 렌더 깜빡거림 지속시간 타이머

    // first interaction hint
    bool isExplainedHideObject = true;      // 튜토리얼용
    bool isExplainedSearchObject = true;    // 튜토리얼용
    bool isExplainedTrapObject = false;
    bool isExplainedCookingZone = false;

    // search object interaction
    bool  isPossibleSearch = false;            // 기획자분이 한번에 하나만 가능한 사이즈라고 하심. 중첩된다면 추가 처리필요.
    SearchObject* curSerachObject = nullptr;   // 현재 search 가능한 오브젝트
    float searchTime  = 2.0f;
    float searchTimer = 0.0f;

    // hide object interaction
    bool  isPossibleHide = false;
    HideObject* curHideObject = nullptr;      // 현재 은신 가능한 오브젝트

    // jesasang interaction
    bool isPossiblePutFood;
    float putFoodTime = 2.0f;
    float putFoodTimer = 0.0f;

    // altar interaction
    bool isPossibleGetFood;
    float getItemAltarTime = 2.0f;
    float getItemAltarTimer = 0.0f;

    // cooking mini game
    bool isPossibleCooking = false;      // 조리대 zone 안에 있는지 flag
    float cookInteractionTime = 2.0f;
    float cookInteractionTimer = 0.0f;


    // --- [ Key ] ---------------------------------
    // key input flags
    bool isMoveLKey;
    bool isMoveRKey;
    bool isMoveFKey;
    bool isMoveBKey;
    bool isSitKey;
    bool isRunKey;
    bool isInteractionKey;

    // key buindings (inspector)
    Keyboard::Keys moveL_Key = Keyboard::Left;
    Keyboard::Keys moveR_Key = Keyboard::Right;
    Keyboard::Keys moveF_Key = Keyboard::Up;
    Keyboard::Keys moveB_Key = Keyboard::Down;

    Keyboard::Keys sit_Key = Keyboard::LeftControl;
    Keyboard::Keys run_Key = Keyboard::LeftShift;
    Keyboard::Keys interaction_Key = Keyboard::F;


public:
    // Component process
    void OnStart() override;
    void OnUpdate(float delta) override;
    void OnFixedUpdate(float delta) override;
    void OnDestory() override;

    // Collsion event
    void OnTriggerEnter(PhysicsComponent*) override;
    void OnTriggerStay(PhysicsComponent*) override;
    void OnTriggerExit(PhysicsComponent*) override;

    void OnCollisionEnter(PhysicsComponent*) override;
    void OnCollisionStay(PhysicsComponent*) override;
    void OnCollisionExit(PhysicsComponent*) override;

    // Json
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);

private:
    // FSM
    void InitFSMStates();

public:
    void ChangeState(PlayerState state); // 선민 | 02.12 ㅎㅎㅎㅎㅎㅎ 

private:
    // Animation
    void LoadAnimation();

    // Init
    void InitStat();

    // Input
    void InputProcess();

    // Movement
    void Move(float delta);
    void Rotation(float delta);

    // Interaction
    void InteractionCheak(float delta);
    void SerachObjectInteraction(float dt);
    void HideObjectInteraction(float dt);
    void CookingInteraction(float dt);
    void PutFoodJesaSangInteraction(float dt);
    void GetItemAltarInteraction(float dt);

    // Hit
    void UpsateHitDuration(float dt);


public:
    // 외부 call Funcs..
    PlayerState GetPlayerState();

    // Intereac Object
    void SetCurSearchObject(SearchObject* object);
    void SetCurHideObject(HideObject* object);


    // MiniGame Return Login
    void ReceiveMiniGameResult(std::unique_ptr<IItem> ingredient, bool isSuccess);
    void ReceiveMiniGameItem(std::unique_ptr<IItem> ingredient);


    // AI
    void TakeAttack();                   // AI에게 공격 당했을 때
    float GetCurSenseRadiuse() const;    // 플레이어 현재 기척 getter


    //// 선민 | 02.12 
    //bool isForcedMove = false;
    //Vector3 forcedDir;
    //float forcedSpeed = 0.0f;

    //void SetForcedMove(const Vector3& dir, float speed);
    //void ClearForcedMove();

    // 게임 종료(성공)시 Stop 로직
    void GameSuceesPlayerStop();

    //---------------------------------------------------------------
    // friend
    friend class IPlayerState;
    friend class Player_Idle;
    friend class Player_Walk;
    friend class Player_Run;
    friend class Player_Sit;
    friend class Player_SitWalk;
    friend class Player_Hide;
    friend class Player_Hit;
    friend class Player_Cook;
    friend class Player_Die;
    friend class InteractionZone;
    friend class InteractionSensor;
    friend class MiniGameManager;
    friend class HideObject;
};

