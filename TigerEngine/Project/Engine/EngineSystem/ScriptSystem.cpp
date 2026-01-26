#include "ScriptSystem.h"
#include "../EngineSystem/PlayModeSystem.h"

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
}

void ScriptSystem::RegisterScript(Component* comp)
{
    scriptComps.push_back(comp);
    scriptCompsInitReadyQueue.push(comp);
    scriptCompsStartReadyQueue.push(comp);
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
        e->OnUpdate(delta);
    }

    if (PlayModeSystem::Instance().IsPlaying())
    {
        // 1. 등록된 init 해소
        while (!scriptCompsInitReadyQueue.empty())
        {
            auto comp = scriptCompsInitReadyQueue.front();
            comp->OnInitialize();
            scriptCompsInitReadyQueue.pop();
        }

        // 2. 등록된 start 해소
        while (!scriptCompsStartReadyQueue.empty())
        {
            auto comp = scriptCompsStartReadyQueue.front();
            comp->OnStart();
            scriptCompsStartReadyQueue.pop();
        }

        // 3. 사용자 정의 component update
        for (auto& e : scriptComps)
        {
            e->OnUpdate(delta);
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
            e->OnFixedUpdate(dt);
        }
    }
}