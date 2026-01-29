#include "RenderSystem.h"
#include "System/TimeSystem.h"
#include "../Object/GameObject.h"

void RenderSystem::CheckReadyQueue()
{
    while (!readyQueue.empty())
    {
        auto comp = readyQueue.front();
        comp->OnStart();
        readyQueue.pop();
    }
}

void RenderSystem::Register(RenderComponent* comp)
{
    readyQueue.push(comp);
    comps.push_back(comp);
    comp->OnInitialize();
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
        if (!e->GetOwner()->GetActiveSelf() || !e->GetActiveSelf()) continue;

        if (!e->IsStart())
        {
            e->IsStart();
            e->SetStartTrue(); // 시작을 알림
        }
        else
        {
            e->OnUpdate(GameTimer::Instance().DeltaTime());
            e->OnRender(queue);
        }
    }
}