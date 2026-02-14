#include "Player_Walk.h"
#include "../../Inventory/Inventory.h"
#include "../../../Ho/Sound/PlayerSoundSource.h"

void Player_Walk::Enter()
{
    //cout << "[Player] Enter Walk State" << endl;

    // set speed
    player->curSpeed = player->walkSpeed;

    // set animation
    player->animController->ChangeState("Walk");

    // set sound
    player->sound->PlayFootStepLoop(FootStepLoopMode::Walk);

    // set sense radius
    player->curSenseRadius = player->walkSenseRadius;
    if (player->inventory->GetCurItemType() == ItemType::Ingredient)
        player->curSenseRadius += player->ingreSenseRadius;
    else if (player->inventory->GetCurItemType() == ItemType::Food)
        player->curSenseRadius += player->foodSenseRadius;
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
    // look dir
    Vector3 input(0, 0, 0);

    if (player->isMoveLKey) input.x -= 1;
    if (player->isMoveRKey) input.x += 1;
    if (player->isMoveFKey) input.z += 1;
    if (player->isMoveBKey) input.z -= 1;

    if (input.LengthSquared() > 0)
        input.Normalize();

    player->lookDir = input;
}

void Player_Walk::FixedUpdate(float deltaTime)
{

}

void Player_Walk::Exit()
{
    //cout << "[Player] Exit Walk State" << endl;
}
