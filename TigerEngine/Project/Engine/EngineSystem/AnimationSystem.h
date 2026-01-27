#pragma once
#include "pch.h"
#include "System/Singleton.h"
#include "../Components/AnimationController.h"

class AnimationSystem : public Singleton<AnimationSystem>
{
private:
    std::vector<AnimationController*> comps{};

public:
    AnimationSystem(token) {}
    ~AnimationSystem() = default;

    void Register(AnimationController* comp);
    void UnRegister(AnimationController* comp);

    void Update(float dt);

    std::vector<AnimationController*> GetComponents();

    /// <summary>
    /// 등록된 AnimationController 컴포넌트들 .clear()로 제거하는 함수
    /// </summary>
    void Clear();
};