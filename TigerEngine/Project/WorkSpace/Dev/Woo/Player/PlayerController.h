#pragma once
#include "../../../../Engine/Components/ScriptComponent.h"
#include "../../../../Engine/Components/FBXRenderer.h"
#include "../../../../Engine/Components/CharacterControllerComponent.h"

#include <directxtk/Keyboard.h>

class IPlayerState;

// Player State Enum
enum class PlayerState
{
    Idle, Walk, Run, Sit, SitWalk, Hide, Hit, Die, None
};

/*
     [ Player Controller Script Component ]

     1) FSM
     2) Input
     3) Movement
     4) TODO :: Interaction...
*/
class PlayerController : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    // --- [ Component ] ---------------------------
    Transform* transform = nullptr;
    FBXRenderer* fbxRenderer = nullptr;
    CharacterControllerComponent* cct = nullptr;

    Transform* camTransform = nullptr;

    // --- [ State ] ---------------------------
    PlayerState state = PlayerState::None;
    IPlayerState* curState;
    IPlayerState* fsmStates[8];

    // --- [ Stat ] --------------------------------
    // current
    float curSpeed = 0.0f;
    Vector3 moveDir = Vector3::Zero;

    // init
    float walkSpeed = 3.0f;
    float runSpeed = 6.0f;
    float sitSpeed = 1.0f;
        

    // --- [ Controll ] ----------------------------


    // --- [ Key ] ---------------------------------
    // key input flags
    bool isMoveLKey;
    bool isMoveRKey;
    bool isMoveFKey;
    bool isMoveBKey;
    bool isSitKey;
    bool isRunKey;
    bool isInteractionKey;

    // key buindings
    Keyboard::Keys moveL_Key = Keyboard::Left;
    Keyboard::Keys moveR_Key = Keyboard::Right;
    Keyboard::Keys moveF_Key = Keyboard::Up;
    Keyboard::Keys moveB_Key = Keyboard::Down;

    Keyboard::Keys sit_Key = Keyboard::LeftControl;
    Keyboard::Keys run_Key = Keyboard::LeftShift;
    Keyboard::Keys interaction_Key = Keyboard::F;


public:
    // Component process
    void OnInitialize() override;
    void OnStart() override;
    void OnUpdate(float delta) override;
    void OnFixedUpdate(float delta) override;
    void OnDestory() override;

    // Collsion event
    void OnCCTTriggerEnter(CharacterControllerComponent*) override;
    void OnCCTTriggerStay(CharacterControllerComponent*) override;
    void OnCCTTriggerExit(CharacterControllerComponent*) override;

    void OnCCTCollisionEnter(CharacterControllerComponent*) override;
    void OnCCTCollisionStay(CharacterControllerComponent*) override;
    void OnCCTCollisionExit(CharacterControllerComponent*) override;

    // Json
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);


public:
    // FSM
    void AddFSMStates();
    void ChangeState(PlayerState state);

    // Init
    void InitStat();

    // Input
    void InputProcess();

    // Movement
    void Move(float delta);
    void Rotation(float delta);

public:
    // 외부 Funcs.. TODO







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
    friend class Player_Die;
};

