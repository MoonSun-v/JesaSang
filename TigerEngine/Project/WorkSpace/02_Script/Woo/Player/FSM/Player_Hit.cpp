#include "Player_Hit.h"
#include "../../Inventory/Inventory.h"
#include "Manager/WorldManager.h"
#include "../../Altar/AltarManager.h"
#include "../../../Ho/Sound/PlayerSoundSource.h"
#include "../../../Ho/Sound/SoundManager.h"

void Player_Hit::Enter()
{
    cout << "[Player] Enter Hit State (life : " << player->curLife << ")" << endl;

    // set speed
    player->curSpeed = player->walkSpeed * player->hitSpeedUpRate;

    // set animation
    player->animController->ChangeState("Hit");

    // sound
    player->sound->PlaySound(PlayerSoundType::Hit, false);
    player->sound->StopFootStep();

    // effect
    player->hitEffect->GetOwner()->SetActive(true);

    // set sense radius
    player->curSenseRadius = player->walkSenseRadius;
    if (player->inventory->GetCurItemType() == ItemType::Ingredient)
        player->curSenseRadius += player->ingreSenseRadius;
    else if (player->inventory->GetCurItemType() == ItemType::Food)
        player->curSenseRadius += player->foodSenseRadius;

    // 3초동안은 무적상태
    if (!player->resumeHitAfterHide)
        player->isPlayerInvincible = true;
    player->resumeHitAfterHide = false;

    // 비네트 On
    if(!AltarManager::Instance()->IsAltarFirstDirecting())
    {
        auto& postProcessData = WorldManager::Instance().postProcessData;
        postProcessData.useVignette = true;
        postProcessData.vignetteColor = { 1,0,0 };
    }

    // timer -> player controller에서 init (hide 중에도 유지하기 위함)
}

void Player_Hit::ChangeStateLogic()
{
    // hit duration 후 자동 change
    if (player->hitTimer >= player->hitDuration)
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
}

void Player_Hit::Update(float deltaTime)
{
    // timer -> player controller에서 update (hide 중에도 유지하기 위함)
    
    // 플레이어 깜빡거리는 연출
    if (player->renderDirectorTimer >= player->renderDirectorTime)
    {
        player->fbxRenderer->SetActive(!player->fbxRenderer->GetActiveSelf());
        player->renderDirectorTimer = 0.0f;
    }

    // look dir
    Vector3 input(0, 0, 0);

    if (player->isMoveLKey) input.x -= 1;
    if (player->isMoveRKey) input.x += 1;
    if (player->isMoveFKey) input.z += 1;
    if (player->isMoveBKey) input.z -= 1;

    if (input.LengthSquared() > 0)
        input.Normalize();

    player->lookDir = -input;       // 이동 반대 (패닉)
}

void Player_Hit::FixedUpdate(float deltaTime)
{

}

void Player_Hit::Exit()
{
    // 무적상태 flag clear
    player->isPlayerInvincible = false;

    // 렌더 다시 on
    player->fbxRenderer->SetActive(true);

    // effect
    player->hitEffect->GetOwner()->SetActive(false);

    // 비네트 Off
    auto& postProcessData = WorldManager::Instance().postProcessData;
    postProcessData.useVignette = false;

    cout << "[Player] Exit Hit State" << endl;
}
