#include "AdultGhost_Attack.h"

#include "EngineSystem/SceneSystem.h"
#include "../../../Woo/Player/PlayerController.h"
#include "../../../Ho/Sound/EnemySoundSource.h"

void AdultGhost_Attack::Enter()
{
    cout << "[AdultGhost_Attack] Enter Attack State" << endl;

    attackTimer = 0.0f;
    didDamage = false;
    isChangeAnim = false;

    adultGhost->animController->ChangeState("Attack");
    adultGhost->enemySound->PlaySound(EnemySoundType::Ghost_Attack_Sound);

    // 이동 완전 정지
    adultGhost->agent->externalControl = true;
    adultGhost->agent->path.clear();
}

void AdultGhost_Attack::ChangeStateLogic()
{
    // 딜레이 이후 전환 로직 실행 ㄱㄱ
    if (attackTimer < attackAnimTime + attackDelayTime)
        return;

    auto* player = adultGhost->GetPlayer();

    if (player && adultGhost->IsSeeing(player))
    {
        cout << "[AdultGhost_Attack] Player still in sight -> Chase\n";
        adultGhost->ChangeState(AdultGhostState::Chase);
    }
    else
    {
        cout << "[AdultGhost_Attack] Lost player -> Search\n";

        auto grid = GridSystem::Instance().GetMainGrid();
        if (grid)
        {
            int px, py;
            auto wp = adultGhost->GetPlayer()->GetTransform()->GetLocalPosition();
            if (grid->WorldToGridFromCenter(wp, px, py))
                adultGhost->lastPlayerGrid = { px, py, true };
        }

        adultGhost->searchReason = SearchReason::FromAttack;
        adultGhost->ChangeState(AdultGhostState::Search);
    }
}

void AdultGhost_Attack::Update(float deltaTime)
{
    attackTimer += deltaTime;

    // 1. Attack -> AttackDelay 애니메이션 전환
    if (!isChangeAnim && attackTimer >= attackAnimTime)
    {
        adultGhost->animController->ChangeState("AttackDelay");
        isChangeAnim = true;
        adultGhost->enemySound->PlaySound(EnemySoundType::Ghost_AttackDelay_Sound);
    }

    // 2. 데미지 타이밍 
    if (!didDamage && attackTimer >= 0.5f)
    {
        auto* player = adultGhost->GetPlayer();
        if (player)
            player->GetComponent<PlayerController>()->TakeAttack();

        didDamage = true;
    }
}

void AdultGhost_Attack::FixedUpdate(float deltaTime)
{
}

void AdultGhost_Attack::Exit()
{
    attackTimer = 0.0f;
    adultGhost->agent->externalControl = false;
}