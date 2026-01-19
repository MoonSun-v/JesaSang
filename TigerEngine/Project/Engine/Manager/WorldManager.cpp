#include "WorldManager.h"
#include "../EngineSystem/CameraSystem.h"

#include "../Components/Camera.h"
#include "../Object/GameObject.h"

void WorldManager::CreateDirectionalLightFrustum()
{
	// TODO main 캠 설정으로 바꿔야함.

#if _DEBUG
	auto camera = CameraSystem::Instance().GetFreeCamera();
#else
	auto camera = CameraSystem::Instance().GetCurrCamera();
#endif

	auto camTran = camera->GetOwner()->GetTransform();
	directionalLightProj = XMMatrixPerspectiveFovLH(directionalLightFrustumAngle, directionalLightViewport.width / (FLOAT)directionalLightViewport.height, directionalLightNear, directionalLightFar); // 그림자 절두체
	directionalLightLookAt = camTran->GetPosition() + camera->GetForward() * directionalLightForwardDistFromCamera;	// 바라보는 방향 = 카메라 위치 + 카메라 바라보는 방향으로부터 떨어진 태양의 위치
	directionalLightPos = camTran->GetPosition() + ((Vector3)-lightDirection * directionalLightUpDistFromLookAt);	// 위치
	directionalLightView = XMMatrixLookAtLH(directionalLightPos, directionalLightLookAt, Vector3(0.0f, 1.0f, 0.0f));
}

void WorldManager::Update()
{

#if _DEBUG
    auto camera = CameraSystem::Instance().GetFreeCamera();
#else
    auto camera = CameraSystem::Instance().GetCurrCamera();
#endif

	auto camTran = camera->GetOwner()->GetTransform();
	directionalLightProj = XMMatrixPerspectiveFovLH(directionalLightFrustumAngle, directionalLightViewport.width / (FLOAT)directionalLightViewport.height, directionalLightNear, directionalLightFar); // 그림자 절두체
	directionalLightLookAt = camTran->GetPosition() + camera->GetForward() * directionalLightForwardDistFromCamera;	// 바라보는 방향 = 카메라 위치 + 카메라 바라보는 방향으로부터 떨어진 태양의 위치
	directionalLightPos = camTran->GetPosition() + ((Vector3)-lightDirection * directionalLightUpDistFromLookAt);	// 위치
	directionalLightView = XMMatrixLookAtLH(directionalLightPos, directionalLightLookAt, Vector3(0.0f, 1.0f, 0.0f));
}

int WorldManager::GetCameraIndex()
{
    return cameraIndex;
}

void WorldManager::SetCameraIndex(int index)
{
    cameraIndex = CameraSystem::Instance().SetCurrCamera(index);
}
