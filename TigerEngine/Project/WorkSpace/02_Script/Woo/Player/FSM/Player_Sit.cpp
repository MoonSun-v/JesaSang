#include "Player_Sit.h"
#include "../../Inventory/Inventory.h"
#include "../../../Ho/Sound/PlayerSoundSource.h"

void Player_Sit::Enter()
{
    //cout << "[Player] Enter Sit State" << endl;

    // set speed
    player->curSpeed = 0;

    // set moved dir
    player->lookDir = Vector3::Zero;

    // set sound
    player->sound->StopFootStep();

    // set animation
    player->animController->ChangeState("Idle");

    // set sense radius
    player->curSenseRadius = player->sitSenseRadius;
    if (player->inventory->GetCurItemType() == ItemType::Ingredient)
        player->curSenseRadius += player->ingreSenseRadius;
    else if (player->inventory->GetCurItemType() == ItemType::Food)
        player->curSenseRadius += player->foodSenseRadius;
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

void Player_Sit::FixedUpdate(float deltaTime)
{
    
}

void Player_Sit::Exit()
{
    //cout << "[Player] Exit Sit State" << endl;
}
