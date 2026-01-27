#include "AnimationController.h"
#include"../Object/GameObject.h"
#include "../Components/FBXData.h"


RTTR_REGISTRATION
{
    rttr::registration::class_<AnimationController>("AnimationController")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr);
}

void AnimationController::OnStart()
{
    // FBXData에서 skeletal info 가져오기
    auto fbx = GetOwner()->GetComponent<FBXData>();
    if (fbx && fbx->GetFBXInfo())
    {
        Initialize(&fbx->GetFBXInfo()->skeletalInfo);  // Animator 초기화
    }

    AnimationSystem::Instance().Register(this);
}

void AnimationController::Initialize(const SkeletonInfo* skeleton)
{
    AnimatorInstance.Initialize(skeleton);
}

void AnimationController::Update(float dt)
{
    if (CurrentState)
    {
        // OutputDebugString((L"[AnimationController] Update dalta : " + std::to_wstring(dt) + L"\n").c_str());
        CurrentState->OnUpdate(dt);
    }

    AnimatorInstance.Update(dt);
}

void AnimationController::OnDestory()
{
    AnimationSystem::Instance().UnRegister(this);
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
        if (anim.m_name == name)
            return &anim;
    }
    return nullptr;
}



nlohmann::json AnimationController::Serialize()
{
    nlohmann::json datas;

    rttr::type t = rttr::type::get(*this);
    datas["type"] = t.get_name().to_string();
    datas["properties"] = nlohmann::json::object(); // 객체 생성

    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);
        if (value.is_type<float>())
        {
            auto v = value.get_value<float>();
            datas["properties"][propName] = v;
        }
    }

    return datas;
}

void AnimationController::Deserialize(nlohmann::json data)
{
    auto propData = data["properties"];

    rttr::type t = rttr::type::get(*this);
    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);
    }
}
