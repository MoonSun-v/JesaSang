#include "Player_Run.h"

void Player_Run::Enter()
{
    cout << "[Player] Enter Run State" << endl;

    // set speed
    player->curSpeed = player->runSpeed;
}

void Player_Run::ChangeStateLogic()
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

    // walk
    if (!player->isRunKey)
    {
        player->ChangeState(PlayerState::Walk);
        return;
    }
}

void Player_Run::Update(float deltaTime)
{
    
}

void Player_Run::FixedUpdate(float deltaTime)
{
   
}

void Player_Run::Exit()
{
    cout << "[Player] Exit Run State" << endl;
}

