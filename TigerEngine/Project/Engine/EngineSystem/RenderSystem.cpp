#include "RenderSystem.h"
#include "System/TimeSystem.h"

void RenderSystem::Register(RenderComponent* comp)
{
    comps.push_back(comp);
}

void RenderSystem::UnRegister(RenderComponent* comp)
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

void RenderSystem::Render(RenderQueue& queue)
{
    for (auto& e : comps)
    {
        e->OnUpdate(GameTimer::Instance().DeltaTime());
        e->OnRender(queue);
    }
}