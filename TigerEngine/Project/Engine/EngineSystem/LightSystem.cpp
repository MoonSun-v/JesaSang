#include "LightSystem.h"

void LightSystem::Register(Light* comp)
{
    comps.push_back(comp);
}

void LightSystem::Update()
{
    // shadow camera matrix update
    Vector3 lightDir;
    for (auto& light : comps)
    {
        if (light->isSunLight)
        {
            lightDir = light->direction;
            break;
        }
    }

    shadowCamera.Udate(camera, lightDir, shadowOrthoDesc);
    shadowView = shadowCamera.GetView();
    shadowProjection = shadowCamera.GetProjection();
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