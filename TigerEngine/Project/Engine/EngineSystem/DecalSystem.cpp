#include "DecalSystem.h"


void DecalSystem::Register(Decal* comp)
{
    comps.push_back(comp);
}

void DecalSystem::UnRegister(Decal* comp)
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

void DecalSystem::Clear()
{
    comps.clear();
}

std::vector<Decal*> DecalSystem::GetComponents()
{
    return comps;
}