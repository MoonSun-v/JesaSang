#include "DirectionalShadowCamera.h"
#include "../../Object/GameObject.h"
#include "../../EngineSystem/LightSystem.h"
#include "../../Manager/WorldManager.h"

void DirectionalShadowCamera::Update(Camera* camera)
{
    ShadowOrthoDesc desc = WorldManager::Instance().shadowData;

    Vector3 lightDir = LightSystem::Instance().GetSunDirection();
    if (lightDir.LengthSquared() < 1e-6f)
        lightDir = Vector3::Down;
    lightDir.Normalize();

    Vector3 camPos = camera->GetOwner()->GetTransform()->GetPosition();

    Vector3 sceneCenter = camPos + camera->GetForward() * desc.lookPointDist;
    Vector3 lightPos = sceneCenter - lightDir * desc.shadowLightDist;

    view = XMMatrixLookAtLH(lightPos, sceneCenter, Vector3::Up);
    projection = XMMatrixOrthographicLH(desc.shadowWidth, desc.shadowHeight, desc.shadowNear, desc.shadowFar);
}
