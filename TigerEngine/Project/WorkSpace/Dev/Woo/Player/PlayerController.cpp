#include "PlayerController.h"
#include "Util/DebugDraw.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"
#include <Util/PhysXUtils.h>

#include "Object/GameObject.h"
#include "System/InputSystem.h"
#include "EngineSystem/PhysicsSystem.h"

#include "FSM/IPlayerState.h"
#include "FSM/Player_Idle.h"
#include "FSM/Player_Walk.h"
#include "FSM/Player_Run.h"
#include "FSM/Player_Sit.h"
#include "FSM/Player_Hide.h"
#include "FSM/Player_Hit.h"
#include "FSM/Player_Die.h"


REGISTER_COMPONENT(PlayerController)

RTTR_REGISTRATION
{
    rttr::registration::class_<PlayerController>("PlayerController")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

/*-------[ Component Process ]-------------------------------------*/
void PlayerController::OnInitialize()
{
    transform = GetOwner()->GetComponent<Transform>();
    fbxRenderer = GetOwner()->GetComponent<FBXRenderer>();
    cct = GetOwner()->GetComponent<CharacterControllerComponent>();
}

void PlayerController::OnStart()
{
    // init fsm
    AddFSMStates();
    ChangeState(PlayerState::Idle);

    // init stat
    InitPlayerStat();
}

void PlayerController::OnUpdate(float delta)
{
    // input
    KeyInputUpdate();
    
    // fsm
    if(curState)
    {
        curState->ChangeStateLogic();
        curState->Update(delta);
    }
}

void PlayerController::OnFixedUpdate(float delta)
{
    // fsm
    if (curState)
    {
        curState->FixedUpdate(delta);
    }
}

void PlayerController::OnDestory()
{

}

/*-------[ Collision Event ]-------------------------------------*/
void PlayerController::OnCCTTriggerEnter(CharacterControllerComponent*)
{

}

void PlayerController::OnCCTTriggerStay(CharacterControllerComponent*)
{

}

void PlayerController::OnCCTTriggerExit(CharacterControllerComponent*)
{
}

void PlayerController::OnCCTCollisionEnter(CharacterControllerComponent*)
{

}

void PlayerController::OnCCTCollisionStay(CharacterControllerComponent*)
{
}

void PlayerController::OnCCTCollisionExit(CharacterControllerComponent*)
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
void PlayerController::AddFSMStates()
{
    fsmStates[(int)PlayerState::Idle] = new Player_Idle(*this);
    fsmStates[(int)PlayerState::Walk] = new Player_Walk(*this);
    fsmStates[(int)PlayerState::Run] = new Player_Run(*this);
    fsmStates[(int)PlayerState::Sit] = new Player_Sit(*this);
    fsmStates[(int)PlayerState::Hide] = new Player_Hide(*this);
    fsmStates[(int)PlayerState::Hit] = new Player_Hit(*this);
    fsmStates[(int)PlayerState::Die] = new Player_Die(*this);
}

void PlayerController::ChangeState(PlayerState state)
{
    if(curState)
        curState->Exit();

    curState = fsmStates[(int)state];
    this->state = state;

    if(curState)
        curState->Enter();
}

/*-------[ Init ]-------------------------------------*/
void PlayerController::InitPlayerStat()
{

}

/*-------[ Input ]-------------------------------------*/
void PlayerController::KeyInputUpdate()
{
    isMoveLKey = Input::GetKey(moveL_Key);
    isMoveRKey = Input::GetKey(moveR_Key);
    isMoveFKey = Input::GetKey(moveF_Key);
    isMoveBKey = Input::GetKey(moveB_Key);
    isSitKey = Input::GetKey(sit_Key);
    isRunKey = Input::GetKey(run_Key);
    isInteractionKey = Input::GetKey(interaction_Key);

    //cout << "L:" << isMoveLKey << " R:" << isMoveRKey << " F:" << isMoveFKey << " B:" << isMoveBKey
    //    << " Sit:" << isSitKey << " Run:" << isRunKey << " Interact:" << isInteractionKey << endl;
}

