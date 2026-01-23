#include "LightSystem.h"

void LightSystem::Register(Light* comp)
{
    comps.push_back(comp);
}

void LightSystem::UnRegister(Light* comp)
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

Vector3 LightSystem::GetSunDirection()
{
    Vector3 lightDir;
    for (auto& light : comps)
    {
        if (light->isSunLight)
        {
            lightDir = light->direction;
            break;
        }
    }

    return lightDir;
}

std::vector<Light*> LightSystem::GetComponents()
{
    return comps;
}

void LightSystem::Clear()
{
    comps.clear();
}
