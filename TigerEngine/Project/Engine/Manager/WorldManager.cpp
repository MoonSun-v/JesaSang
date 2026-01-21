#include "WorldManager.h"
#include "ShaderManager.h"
#include "../Components/Camera.h"
#include "../Object/GameObject.h"
#include "../../Base/System/TimeSystem.h"

RTTR_REGISTRATION
{
    using namespace rttr;

    registration::class_<WorldManager>("WorldManager")
        // Singleton(token) 때문에 ctor 등록은 생략하는 게 안전함.
        // .constructor<>() 안 달아도 instance 기반 property 접근은 가능.

        // World Light Data
        .property("useIBL",             &WorldManager::useIBL)
        .property("indirectIntensity",  &WorldManager::indirectIntensity)

        // PostProcess / Frame
        .property("postProcessData",    &WorldManager::postProcessData)
        .property("frameData",          &WorldManager::frameData);
}

void WorldManager::Update(const ComPtr<ID3D11DeviceContext>& context, Camera* camera,
    int clientWidth, int clientHeight)
{
    auto& sm = ShaderManager::Instance();

    // Frame CB Update
    sm.frameCBData.time = GameTimer::Instance().TotalTime();
    sm.frameCBData.deltaTime = GameTimer::Instance().DeltaTime();
    sm.frameCBData.screenSize = { (float)clientWidth,(float)clientHeight };
    sm.frameCBData.cameraPos = camera->GetOwner()->GetTransform()->GetPosition();
    context->UpdateSubresource(sm.frameCB.Get(), 0, nullptr, &sm.frameCBData, 0, 0);
}

//int WorldManager::GetCameraIndex()
//{
//    return cameraIndex;
//}
//
//void WorldManager::SetCameraIndex(int index)
//{
//    cameraIndex = CameraSystem::Instance().SetCurrCamera(index);
//}
