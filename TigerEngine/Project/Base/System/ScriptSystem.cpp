#include "ScriptSystem.h"

void ScriptSystem::Register(Component* comp)
{
    comps.push_back(comp);
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

void ScriptSystem::Update(float delta)
{
    for (auto& e : comps)
    {
        e->OnUpdate(delta);
    }
}
