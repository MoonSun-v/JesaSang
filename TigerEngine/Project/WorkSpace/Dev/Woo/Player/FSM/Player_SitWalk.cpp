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
    // input dir
    Vector3 input(0, 0, 0);

    if (player->isMoveLKey) input.x -= 1;
    if (player->isMoveRKey) input.x += 1;
    if (player->isMoveFKey) input.z += 1;
    if (player->isMoveBKey) input.z -= 1;

    if (input.LengthSquared() > 0)
        input.Normalize();

    // move dir
    float yaw = player->GetOwner()->GetTransform()->GetYaw();

    Vector3 forward = { sinf(yaw), 0, cosf(yaw) };
    Vector3 right = { cosf(yaw), 0,-sinf(yaw) };

    Vector3 moveDir = forward * input.z + right * input.x;
    if (moveDir.LengthSquared() > 0)
        moveDir.Normalize();

    player->moveDir = moveDir;
}

void Player_SitWalk::FixedUpdate(float deltaTime)
{
}

void Player_SitWalk::Exit()
{
    cout << "[Player] Exit SitWalk State" << endl;
}
