#include "AnimationController.h"
#include"../Object/GameObject.h"
#include "../Components/FBXData.h"
#include "../Util/JsonHelper.h"


RTTR_REGISTRATION
{
    rttr::registration::class_<AnimationController>("AnimationController")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr);
}

nlohmann::json AnimationController::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void AnimationController::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void AnimationController::Enable_Inner()
{
    ScriptSystem::Instance().Register(this); // AnimationSystem X 
    OnEnable();
}

void AnimationController::Disable_Inner()
{
    ScriptSystem::Instance().UnRegister(this); // AnimationSystem X 
    OnDisable();
}


// ------------------------------------------------------

void AnimationController::OnInitialize()
{
    auto fbx = GetOwner()->GetComponent<FBXData>();
    auto asset = fbx ? fbx->GetFBXInfo() : nullptr;
    cachedAsset = asset.get();
    AnimatorInstance.Initialize(asset ? &asset->skeletalInfo : nullptr); // Animator ???
}


void AnimationController::OnUpdate(float delta)
{
    // FBX ???? ???? ?? ??? ??? ??
    auto fbx = GetOwner()->GetComponent<FBXData>();
    auto asset = fbx ? fbx->GetFBXInfo() : nullptr;
    const FBXResourceAsset* assetPtr = asset.get();
    if (assetPtr != cachedAsset)
    {
        cachedAsset = assetPtr;
        States.clear();
        CurrentState = nullptr;
        AnimatorInstance.Initialize(asset ? &asset->skeletalInfo : nullptr);
    }

    if (CurrentState)
    {
        CurrentState->OnUpdate(delta);
    }

    AnimatorInstance.Update(delta);
}


void AnimationController::AddState(std::unique_ptr<AnimationState> state)
{
    state->Controller = this;
    States[state->Name] = std::move(state);
}


void AnimationController::ChangeState(const std::string& name, float blendTime)
{
    if (CurrentState && CurrentState->Name == name)
        return;

    if (CurrentState)
        CurrentState->OnExit();

    auto it = States.find(name);
    if (it == States.end())
        return;

    CurrentState = it->second.get();
    CurrentState->OnEnter();

    if (CurrentState->Clip)
        AnimatorInstance.Play(CurrentState->Clip, blendTime);
    else
        OutputDebugStringW(L"[AnimationController] Warning: CurrentState->Clip is null!\n");
}


const Animation* AnimationController::FindClip(const std::string& name)
{
    auto fbx = GetOwner()->GetComponent<FBXData>();
    if (!fbx) return nullptr;

    auto asset = fbx->GetFBXInfo();
    if (!asset) return nullptr;

    for (auto& anim : asset->animations)
    {
        if (anim && anim->m_name == name)
            return anim.get();
    }
    return nullptr;
}
