#include "Player_SitWalk.h"

void Player_SitWalk::Enter()
{
    cout << "[Player] Enter SitWalk State" << endl;

    // set speed
    player->curSpeed = player->sitSpeed;
}

void Player_SitWalk::ChangeStateLogic()
{
    const bool isMove =
        player->isMoveLKey || player->isMoveRKey ||
        player->isMoveFKey || player->isMoveBKey;

    // idle, walk, run
    if (!player->isSitKey)
    {
        if (isMove)
            player->ChangeState(player->isRunKey ? PlayerState::Run : PlayerState::Walk);
        else
            player->ChangeState(PlayerState::Idle);

        return;
    }

    // sit
    if (!isMove)
    {
        player->ChangeState(PlayerState::Sit);
        return;
    }
}

void Player_SitWalk::Update(float deltaTime)
{
    
}

void Player_SitWalk::FixedUpdate(float deltaTime)
{
}

void Player_SitWalk::Exit()
{
    cout << "[Player] Exit SitWalk State" << endl;
}
