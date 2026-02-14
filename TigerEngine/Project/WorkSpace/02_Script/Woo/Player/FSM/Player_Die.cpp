#include "Player_Die.h"
#include "../../Inventory/Inventory.h"
#include "../../../Ho/Sound/PlayerSoundSource.h"

void Player_Die::Enter()
{
    cout << "[Player] Enter Die State" << endl;

    // set speed
    player->curSpeed = 0;

    // set sense radius
    player->curSenseRadius = 0;

    // set animation
    player->animController->ChangeState("Hit");

    // set sound
    player->sound->StopFootStep();
    player->sound->PlaySound(PlayerSoundType::HpDown, false);
}

void Player_Die::ChangeStateLogic()
{

}

void Player_Die::Update(float deltaTime)
{
    // TODO :: 죽는 연출 후 씬 전환 등?
}

void Player_Die::FixedUpdate(float deltaTime)
{

}

void Player_Die::Exit()
{
    cout << "[Player] Exit Die State" << endl;
}
