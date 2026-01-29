#include "AnimationSystem.h"

void AnimationSystem::Register(AnimationController* comp)
{
    comps.push_back(comp);
}

void AnimationSystem::UnRegister(AnimationController* comp)
{
    for (auto it = comps.begin(); it != comps.end(); it++)
    {
        if (*it == comp)
        {
            comps.erase(it);
            return;
        }
    }
}

void AnimationSystem::Update(float dt)
{
    for (auto* controller : comps)
    {

        if (controller)
            controller->Update(dt);
    }
}

std::vector<AnimationController*> AnimationSystem::GetComponents()
{
    return comps;
}

void AnimationSystem::Clear()
{
    comps.clear();
}