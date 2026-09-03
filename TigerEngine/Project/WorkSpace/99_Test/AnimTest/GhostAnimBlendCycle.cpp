#include "GhostAnimBlendCycle.h"

#include "Object/GameObject.h"
#include "Components/FBXData.h"
#include "Components/AnimationController.h"
#include "Util/AnimationState.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

REGISTER_COMPONENT(GhostAnimBlendCycle)

RTTR_REGISTRATION
{
    rttr::registration::class_<GhostAnimBlendCycle>("GhostAnimBlendCycle")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr)

        .property("IdleHoldTime", &GhostAnimBlendCycle::IdleHoldTime)
        .property("CryHoldTime", &GhostAnimBlendCycle::CryHoldTime)
        .property("UseBlending", &GhostAnimBlendCycle::UseBlending)
        .property("BlendDuration", &GhostAnimBlendCycle::BlendDuration);
}

nlohmann::json GhostAnimBlendCycle::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void GhostAnimBlendCycle::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void GhostAnimBlendCycle::OnStart()
{
    fbxData = GetOwner()->GetComponent<FBXData>();
    animController = GetOwner()->GetComponent<AnimationController>();

    if (!fbxData || !animController)
    {
        std::cout << "[GhostAnimBlendCycle] Component Missing (FBXData / AnimationController)\n";
        return;
    }

    LoadAnimations();

    isCrying = false;
    timer = 0.0f;

    // 최초 진입은 재생 중인 클립이 없어서 블렌딩 여부와 상관없이 바로 재생된다.
    float blendTime = UseBlending ? BlendDuration : 0.0f;
    animController->ChangeState("Idle", blendTime);

    std::cout << "[GhostAnimBlendCycle] " << GetOwner()->GetName()
        << " -> UseBlending : " << UseBlending << "\n";
}

void GhostAnimBlendCycle::OnUpdate(float delta)
{
    if (!animController) return;

    timer += delta;

    float holdTime = isCrying ? CryHoldTime : IdleHoldTime;
    if (timer >= holdTime)
    {
        timer = 0.0f;
        SwitchState();
    }
}

void GhostAnimBlendCycle::SwitchState()
{
    isCrying = !isCrying;

    // UseBlending이 꺼져있으면 blendTime 0으로 넘겨서 즉시 컷 전환(TRS 블렌딩 미적용)되게 한다.
    float blendTime = UseBlending ? BlendDuration : 0.0f;
    animController->ChangeState(isCrying ? "Cry" : "Idle", blendTime);
}

void GhostAnimBlendCycle::LoadAnimations()
{
    auto asset = fbxData->GetFBXInfo();
    if (!asset) return;

    // 04_MainGameScene의 BabyGhostController(LoadAnimation)가 실제로 사용하는
    // 애니메이션(Idle / Cry)만 동일하게 로드한다.
    FBXResourceManager::Instance().LoadAnimationByPath(asset, "..\\Assets\\Resource\\Animation\\Baby_Ghost\\ani_idle_babyghost.fbx", "Idle");
    FBXResourceManager::Instance().LoadAnimationByPath(asset, "..\\Assets\\Resource\\Animation\\Baby_Ghost\\ani_cry_babyghost.fbx", "Cry");

    auto idleClip = animController->FindClip("Idle");
    auto cryClip = animController->FindClip("Cry");

    if (idleClip)
        animController->AddState(std::make_unique<AnimationState>("Idle", idleClip, animController));
    if (cryClip)
        animController->AddState(std::make_unique<AnimationState>("Cry", cryClip, animController));

    if (!idleClip || !cryClip)
    {
        std::cout << "[GhostAnimBlendCycle] Clip not found! (Idle:" << (bool)idleClip
            << " Cry:" << (bool)cryClip << ")\n";
    }
}
