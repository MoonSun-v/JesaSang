#include "Player_Idle.h"
#include "../../Inventory/Inventory.h"
#include "../../../Ho/Sound/PlayerSoundSource.h"

void Player_Idle::Enter()
{
    //cout << "[Player] Enter Idle State" << endl;

    // set speed
    player->curSpeed = 0;

    // set animation
    player->animController->ChangeState("Idle");

    // set sound
    player->sound->StopFootStep();

    // set sense radius
    player->curSenseRadius = player->idleSenseRadius;
    if (player->inventory->GetCurItemType() == ItemType::Ingredient)
        player->curSenseRadius += player->ingreSenseRadius;
    else if (player->inventory->GetCurItemType() == ItemType::Food)
        player->curSenseRadius += player->foodSenseRadius;
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

void Player_Idle::FixedUpdate(float deltaTime)
{

}

void Player_Idle::Exit()
{
    //cout << "[Player] Exit Idle State" << endl;
}
