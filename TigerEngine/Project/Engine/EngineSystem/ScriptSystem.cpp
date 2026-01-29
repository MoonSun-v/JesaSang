#include "ScriptSystem.h"
#include "../EngineSystem/PlayModeSystem.h"
#include "../Object/GameObject.h"

void ScriptSystem::CheckReadyQueue()
{
    while (!readyQueue.empty())
    {
        auto comp = readyQueue.front();
        comp->OnStart();
        readyQueue.pop();
    }
}

void ScriptSystem::Register(Component* comp)
{
    readyQueue.push(comp);
    comps.push_back(comp);
    comp->OnInitialize();
}

void ScriptSystem::RegisterScript(Component* comp)
{
    scriptComps.push_back(comp);
    scriptCompsInitReady.push_back(comp);
}

void ScriptSystem::UnRegister(Component* comp)
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

void ScriptSystem::UnRegisterScript(Component* comp)
{
    for (auto it = scriptComps.begin(); it != scriptComps.end(); it++)
    {
        if (*it == comp)
        {
            scriptComps.erase(it);
            return;
        }
    }
}

void ScriptSystem::Update(float delta)
{
    // 일반 component update
    for (auto& e : comps)
    {
        if (!e->GetOwner()->GetActiveSelf()) continue;  // Object 활성화 여부
        if (!e->GetActiveSelf()) continue;              // 컴포넌트 활성화 여부

        if (!e->IsStart())
        {
            e->IsStart();
            e->SetStartTrue();
        }
        else
        {
            e->OnUpdate(delta);
        }
    }

    if (PlayModeSystem::Instance().IsPlaying())
    {
        // 1. 등록된 init 해소

        for (auto it = scriptCompsInitReady.begin(); it != scriptCompsInitReady.end();)
        {
            if (!(*it)->GetOwner()->GetActiveSelf() || !(*it)->GetActiveSelf())
            {
                it++;
            }
            else
            {
                (*it)->OnInitialize();
                it = scriptCompsInitReady.erase(it);
            }
        }

        // 3. 사용자 정의 component update
        for (auto& e : scriptComps)
        {
            if (!e->GetOwner()->GetActiveSelf() || !e->GetActiveSelf()) continue;
                
            if (!e->IsStart())
            {
                e->OnStart();
                e->SetStartTrue();
            }
            else
            {
                e->OnUpdate(delta);
            }
        }
    }
}

void ScriptSystem::FixedUpdate(float dt)
{
    if (PlayModeSystem::Instance().IsPlaying())
    {
        // 사용자 정의 component update
        for (auto& e : scriptComps)
        {
            if (!e->GetOwner()->GetActiveSelf() || !e->GetActiveSelf()) continue;

            e->OnFixedUpdate(dt);
        }
    }
}

void ScriptSystem::LateUpdate(float dt)
{
    if (PlayModeSystem::Instance().IsPlaying())
    {
        // 사용자 정의 component update
        for (auto& e : scriptComps)
        {
            if (!e->GetOwner()->GetActiveSelf() || !e->GetActiveSelf()) continue;

            e->OnLateUpdate(dt);
        }
    }
}
