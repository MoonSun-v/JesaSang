#include "Player_SitWalk.h"
#include "../../Inventory/Inventory.h"
#include "../../../Ho/Sound/PlayerSoundSource.h"

void Player_SitWalk::Enter()
{
    //cout << "[Player] Enter SitWalk State" << endl;

    // set speed
    player->curSpeed = player->sitSpeed;

    // set animation
    player->animController->ChangeState("Sit");

    // set sound
    player->sound->PlayFootStepLoop(FootStepLoopMode::SlowWalk);
    
    // set sense radius
    player->curSenseRadius = player->sitSenseRadius;
    if (player->inventory->GetCurItemType() == ItemType::Ingredient)
        player->curSenseRadius += player->ingreSenseRadius;
    else if (player->inventory->GetCurItemType() == ItemType::Food)
        player->curSenseRadius += player->foodSenseRadius;
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

void Player_SitWalk::FixedUpdate(float deltaTime)
{
}

void Player_SitWalk::Exit()
{
    //cout << "[Player] Exit SitWalk State" << endl;
}
