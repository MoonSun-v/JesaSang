#include "GhostAnimShowcase.h"

#include "Object/GameObject.h"
#include "Components/FBXData.h"
#include "Components/AnimationController.h"
#include "Util/AnimationState.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

REGISTER_COMPONENT(GhostAnimShowcase)

RTTR_REGISTRATION
{
    rttr::registration::class_<GhostAnimShowcase>("GhostAnimShowcase")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr)

        .property("PlayState", &GhostAnimShowcase::PlayState);
}

nlohmann::json GhostAnimShowcase::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void GhostAnimShowcase::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void GhostAnimShowcase::OnStart()
{
    fbxData = GetOwner()->GetComponent<FBXData>();
    animController = GetOwner()->GetComponent<AnimationController>();

    if (!fbxData || !animController)
    {
        std::cout << "[GhostAnimShowcase] Component Missing (FBXData / AnimationController)\n";
        return;
    }

    LoadAnimations();

    // 촬영용이라 블렌딩 없이 바로 해당 포즈/애니메이션으로 전환
    animController->ChangeState(PlayState, 0.0f);

    std::cout << "[GhostAnimShowcase] " << GetOwner()->GetName()
        << " -> PlayState : " << PlayState << "\n";
}

void GhostAnimShowcase::LoadAnimations()
{
    auto asset = fbxData->GetFBXInfo();
    if (!asset) return;

    // 어른 귀신은 별도의 Walk(이동) 애니메이션이 없다.
    // 04_MainGameScene의 AdultGhostController(LoadAnimation)가 실제로 사용하는
    // 애니메이션(Idle / Attack / AttackDelay)만 동일하게 로드한다.
    FBXResourceManager::Instance().LoadAnimationByPath(asset, "..\\Assets\\Resource\\Animation\\Adult_Ghost\\ani_idle_ghost.fbx", "Idle");
    FBXResourceManager::Instance().LoadAnimationByPath(asset, "..\\Assets\\Resource\\Animation\\Adult_Ghost\\ani_attack_ghost.fbx", "Attack", true);
    FBXResourceManager::Instance().LoadAnimationByPath(asset, "..\\Assets\\Resource\\Animation\\Adult_Ghost\\ani_attackdelay_ghost.fbx", "AttackDelay", true);

    auto idleClip = animController->FindClip("Idle");
    auto attackClip = animController->FindClip("Attack");
    auto attackDelayClip = animController->FindClip("AttackDelay");

    if (idleClip)
        animController->AddState(std::make_unique<AnimationState>("Idle", idleClip, animController));
    if (attackClip)
        animController->AddState(std::make_unique<AnimationState>("Attack", attackClip, animController));
    if (attackDelayClip)
        animController->AddState(std::make_unique<AnimationState>("AttackDelay", attackDelayClip, animController));

    if (!idleClip || !attackClip || !attackDelayClip)
    {
        std::cout << "[GhostAnimShowcase] Clip not found! (Idle:" << (bool)idleClip
            << " Attack:" << (bool)attackClip << " AttackDelay:" << (bool)attackDelayClip << ")\n";
    }
}
