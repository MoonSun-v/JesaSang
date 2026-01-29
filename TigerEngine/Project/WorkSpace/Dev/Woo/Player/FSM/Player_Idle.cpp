#include "Player_Idle.h"

void Player_Idle::Enter()
{
    cout << "[Player] Enter Idle State" << endl;

    // set speed
    player->curSpeed = 0;

    // set moved dir
    player->moveDir = Vector3::Zero;
}

void Player_Idle::ChangeStateLogic()
{
    const bool isMove =
        player->isMoveLKey || player->isMoveRKey ||
        player->isMoveFKey || player->isMoveBKey;

    // sit, sit walk
    if (player->isSitKey)
    {
        player->ChangeState(isMove? PlayerState::SitWalk : PlayerState::Sit);
        return;
    }

    // run, walk
    if (isMove)
    {
        if (player->isRunKey)
            player->ChangeState(PlayerState::Run);
        else
            player->ChangeState(PlayerState::Walk);

        return;
    }
}

void Player_Idle::Update(float deltaTime)
{
   
}

void Player_Idle::FixedUpdate(float deltaTime)
{

}

void Player_Idle::Exit()
{
    cout << "[Player] Exit Idle State" << endl;
}
