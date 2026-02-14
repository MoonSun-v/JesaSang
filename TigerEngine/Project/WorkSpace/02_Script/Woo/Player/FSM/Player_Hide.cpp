#include "Player_Hide.h"
#include "../../Player/PlayerItemVisualizer.h"
#include "System/InputSystem.h"
#include "../../Camera/CameraController.h"
#include "../../Object/HideObject.h"
#include "../../Inventory/Inventory.h"
#include "../../../Ho/Sound/PlayerSoundSource.h"
#include "../../../Ho/Sound/SoundManager.h"

void Player_Hide::Enter()
{
    // set animation
    player->animController->ChangeState("Idle");

    // player render off
    player->fbxRenderer->SetActive(false);
    player->visualizer->VisualRenderOff();

    // camera view mode change
    player->camController->SetTargetTransform(player->curHideObject->GetOwner()->GetTransform());
    player->camController->SetViewMode(CameraController::ViewMode::Top);

    // set speed
    player->curSpeed = 0;

    // set sense radius
    player->curSenseRadius = 0;

    // set sound
    player->sound->StopFootStep();
    SoundManager::Instance()->PlaySFX(SFXType::HiddenObj_Playerin_Sound);

    //cout << "[Player] Enter Hide State" << endl;
}

void Player_Hide::ChangeStateLogic()
{
    if (Input::GetKeyDown(player->interaction_Key))
    {
        player->curHideObject->StopHide();

        // Hit에서 Hide로 들어온 경우에만 Hit로 복귀
        if (player->resumeHitAfterHide && player->hitTimer < player->hitDuration)
            player->ChangeState(PlayerState::Hit);
        else
            player->ChangeState(PlayerState::Idle);

        player->resumeHitAfterHide = false;
    }
}

void Player_Hide::Update(float deltaTime)
{

}

void Player_Hide::FixedUpdate(float deltaTime)
{

}

void Player_Hide::Exit()
{
    // player render on
    player->fbxRenderer->SetActive(true);
    player->visualizer->VisualRenderOn();

    // camera view mode change
    player->camController->SetTargetTransform(player->transform);
    player->camController->SetViewMode(CameraController::ViewMode::Quarter);

    // set sound
    SoundManager::Instance()->PlaySFX(SFXType::HiddenObj_Playerin_Sound);

    //cout << "[Player] Exit Hide State" << endl;
}
