#include "WorldManager.h"
#include "ShaderManager.h"
#include "../Components/Camera.h"
#include "../Object/GameObject.h"
#include "../../Base/System/TimeSystem.h"


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
