#pragma once
#include "../Util/Animator.h"
#include "../Util/AnimationState.h"
#include "../Util/AnimatorParameter.h"
#include "../EngineSystem/AnimationSystem.h"

struct FBXResourceAsset;

// ----------------------------------------------------
// [ AnimationController ] 
// 
// 애니메이션 FSM의 중심 컨트롤러
// 현재 상태를 관리하고 상태 전환에 따라 Animator에 애니메이션 재생
// ----------------------------------------------------

class AnimationController : public Component
{
    RTTR_ENABLE(Component)

public:
    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

private:
    std::unordered_map<std::string, std::unique_ptr<AnimationState>> States;
    AnimationState* CurrentState = nullptr;
    const FBXResourceAsset* cachedAsset = nullptr;

public:
    Animator AnimatorInstance; // 실제 애니메이션 재생기
    AnimatorParameter Params;

    void OnInitialize() override;
    void OnUpdate(float delta) override;
    void Enable_Inner() override;
    void Disable_Inner() override;

    void AddState(std::unique_ptr<AnimationState> state);
    void ChangeState(const std::string& name, float blendTime = 0.2f);
    const Animation* FindClip(const std::string& name);

    AnimationState* GetCurrentState() const { return CurrentState; }



public: 

    // States 읽기용 getter
    const std::unordered_map<std::string, std::unique_ptr<AnimationState>>& GetStates() const
    {
        return States;
    }

    // 등록된 상태 이름 리스트 가져오기
    std::vector<std::string> GetStateNames() const
    {
        std::vector<std::string> names;
        for (auto& pair : States)
            names.push_back(pair.first);
        return names;
    }

    // 특정 상태 객체 가져오기
    AnimationState* GetState(const std::string& name) const
    {
        auto it = States.find(name);
        return it != States.end() ? it->second.get() : nullptr;
    }
};
