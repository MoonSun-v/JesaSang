#include "Player_Walk.h"

void Player_Walk::Enter()
{
    cout << "[Player] Enter Walk State" << endl;

    // set speed
    player->curSpeed = player->walkSpeed;
}

void Player_Walk::ChangeStateLogic()
{
    const bool isMove =
        player->isMoveLKey || player->isMoveRKey ||
        player->isMoveFKey || player->isMoveBKey;

    // sit, sit walk
    if (player->isSitKey)
    {
        player->ChangeState(isMove ? PlayerState::SitWalk : PlayerState::Sit);
        return;
    }

    // idle
    if (!isMove)
    {
        player->ChangeState(PlayerState::Idle);
        return;
    }

    // run
    if (player->isRunKey)
    {
        player->ChangeState(PlayerState::Run);
        return;
    }
}

void Player_Walk::Update(float deltaTime)
{
   
}

void Player_Walk::FixedUpdate(float deltaTime)
{

}

void Player_Walk::Exit()
{
    cout << "[Player] Exit Walk State" << endl;
}
