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

    // sit
    if (player->isSitKey)
    {
        player->ChangeState(PlayerState::Sit);
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

void Player_Run::FixedUpdate(float deltaTime)
{
   
}

void Player_Run::Exit()
{
    cout << "[Player] Exit Run State" << endl;
}

