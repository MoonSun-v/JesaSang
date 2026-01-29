#include "Player_Sit.h"

void Player_Sit::Enter()
{
    cout << "[Player] Enter Sit State" << endl;

    // set speed
    player->curSpeed = 0;

    // set moved dir
    player->moveDir = Vector3::Zero;
}

void Player_Sit::ChangeStateLogic()
{
    // idle
    if (!player->isSitKey)
    {
        player->ChangeState(PlayerState::Idle);
        return;
    }

    const bool isMove =
        player->isMoveLKey || player->isMoveRKey ||
        player->isMoveFKey || player->isMoveBKey;

    // sit walk
    if (isMove)
    {
        player->ChangeState(PlayerState::SitWalk);
        return;
    }
}

void Player_Sit::Update(float deltaTime)
{

}

void Player_Sit::FixedUpdate(float deltaTime)
{
    
}

void Player_Sit::Exit()
{
    cout << "[Player] Exit Sit State" << endl;
}
