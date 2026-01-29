#include "Player_Sit.h"

void Player_Sit::Enter()
{
    cout << "[Player] Enter Sit State" << endl;

    // set speed
    player->curSpeed = player->sitSpeed;
}

void Player_Sit::ChangeStateLogic()
{
    // idle, walk, run
    if (!player->isSitKey)
    {
        // TODO :: sit walk 추가
        player->ChangeState(PlayerState::Idle);
    }

}

void Player_Sit::Update(float deltaTime)
{

}

void Player_Sit::FixedUpdate(float deltaTime)
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

void Player_Sit::Exit()
{
    cout << "[Player] Exit Sit State" << endl;
}
