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

void LightSystem::Clear()
{
    comps.clear();
}

std::vector<Light*> LightSystem::GetComponents()
{
    return comps;
}

Vector3 LightSystem::GetSunDirection()
{
    Vector3 lightDir;
    bool foundSunLight = false;

    for (auto& light : comps)
    {
        if (light->isSunLight)
        {
            lightDir = light->direction;
            foundSunLight = true;
            break;
        }
    }

    if (!foundSunLight)
        lightDir = Vector3(0, 0, 1);       // 임시

    return lightDir;
}
