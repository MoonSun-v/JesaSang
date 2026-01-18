#include "../System/CameraSystem.h"
#include "../Entity/GameObject.h"
#include "../Entity/Camera.h"

Camera *CameraSystem::GetFreeCamera()
{
    return freeCamera;
}

void CameraSystem::Register(Camera *cam)
{
    registered.push_back(cam);
    mappedRegistered[mappedRegistered.size()] = cam;
}

void CameraSystem::RemoveCamera(Camera *cam)
{
    // vector에서 제거
    for(auto it = registered.begin(); it != registered.end();)
    {
        if(*it == cam)
        {
            auto vback = registered.back();
            registered.back() = *it;
            *it = vback;
            registered.pop_back();
        }
        it++;
    }

    // map에서 제거
    for (auto it = mappedRegistered.begin(); it != mappedRegistered.end(); it++)
    {
        if (it->second == cam)
        {
            mappedRegistered.erase(it);
            break;
        }
    }
}

int CameraSystem::SetCurrCamera(int index)
{
    if (index < 0 || index > registered.size()) // 잘못된 인덱스 선택 방지
    {
        currCameraIndex = 0;
    }
    else
    {
        currCameraIndex = index;
    }

    return currCameraIndex;
}

Camera *CameraSystem::GetCameraByIndex(int index)
{
    if(auto it = mappedRegistered.find(index); it != mappedRegistered.end())
    {
        return it->second;
    }
    else
    {

        return nullptr;
    }
}

void CameraSystem::Clear()
{
    registered.clear();
    mappedRegistered.clear();
}

void CameraSystem::SetScreenSize(int width, int height)
{
    screenWidth = width;
    screenHeight = height;
}

void CameraSystem::FreeCameraUpdate(float delta)
{
    if(!freeCamObj) return;
    auto comps = freeCamObj->GetComponents();
    for(auto& comp : comps)
    {
        comp->OnUpdate(delta);
    }
}

void CameraSystem::CreateFreeCamera(int clientWidth, int clientHeight, Scene *currScene)
{
    // TODO 카메라 씬에 등록하기 아니면 다른 업데이트 방법 찾기
    Handle handle = ObjectSystem::Instance().Create<GameObject>();
    freeCamObj = ObjectSystem::Instance().Get<GameObject>(handle);
	freeCamObj->SetName("FreeCamera");
    freeCamera = freeCamObj->AddComponent<Camera>();

	freeCamera->SetProjection(DirectX::XM_PIDIV2, clientWidth, clientHeight, 0.1, 3000);
}
