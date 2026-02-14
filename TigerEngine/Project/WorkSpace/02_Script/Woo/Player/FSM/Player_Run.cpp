#include "Player_Run.h"
#include "../../Inventory/Inventory.h"
#include "../../../Ho/Sound/PlayerSoundSource.h"

void Player_Run::Enter()
{
    //cout << "[Player] Enter Run State" << endl;

    // set speed
    player->curSpeed = player->runSpeed;

    // set animation
    player->animController->ChangeState("Run");

    // set sound
    player->sound->PlayFootStepLoop(FootStepLoopMode::Run);

    // set sense radius
    player->curSenseRadius = player->runSenseRadius;
    if (player->inventory->GetCurItemType() == ItemType::Ingredient)
        player->curSenseRadius += player->ingreSenseRadius;
    else if (player->inventory->GetCurItemType() == ItemType::Food)
        player->curSenseRadius += player->foodSenseRadius;
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

void Player_Run::FixedUpdate(float deltaTime)
{
   
}

void Player_Run::Exit()
{
    //cout << "[Player] Exit Run State" << endl;
}

