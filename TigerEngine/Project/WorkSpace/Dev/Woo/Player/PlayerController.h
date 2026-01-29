#pragma once
#include "../../../../Engine/Components/ScriptComponent.h"
#include "../../../../Engine/Components/FBXRenderer.h"
#include "../../../../Engine/Components/CharacterControllerComponent.h"

#include <directxtk/Keyboard.h>

class IPlayerState;

// Player State Enum
enum class PlayerState
{
    Idle, Walk, Run, Sit, Hide, Hit, Die
};

// Player Controller Script Component
class PlayerController : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    // --- [ Component ] ---------------------------
    Transform* transform = nullptr;
    FBXRenderer* fbxRenderer = nullptr;
    CharacterControllerComponent* cct = nullptr;

    // --- [ State ] ---------------------------
    PlayerState state = PlayerState::Idle;
    IPlayerState* curState;
    IPlayerState* fsmStates[7];

    // --- [ Stat ] --------------------------------
    // current
    float curSpeed;

    // init
    float speed;
    float sitSpeed;
    float runSpeed;

    // --- [ State Flags ] -------------------------


    // --- [ Controll ] ----------------------------


    // --- [ Key Input Flags ] ---------------------
    bool isMoveLKey;
    bool isMoveRKey;
    bool isMoveFKey;
    bool isMoveBKey;
    bool isSitKey;
    bool isRunKey;
    bool isInteractionKey;

    // --- [ Key Bild ] ----------------------------
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

    // Key Input Handler
    void KeyInputUpdate();
    
    // Initialize Player Stat
    void InitPlayerStat();

public:
    // 외부 Funcs.. TODO
};

