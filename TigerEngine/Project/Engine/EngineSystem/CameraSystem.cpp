#include "CameraSystem.h"
#include "../Object/GameObject.h"
#include "../Components/Camera.h"


CameraSystem::CameraSystem(token)
{
    lightCamera = new DirectionalShadowCamera();
}

CameraSystem::~CameraSystem()
{
    delete lightCamera;
}

Camera *CameraSystem::GetFreeCamera()
{
    return freeCamera;
}

void CameraSystem::Register(Camera *cam)
{
    registered.push_back(cam);
    mappedRegistered[mappedRegistered.size()] = cam;
    cout << cam->GetName() << "cam register\n";
}

void CameraSystem::RemoveCamera(Camera *cam)
{
    if (cam->GetOwner()->GetName() == "FreeCamera") return; // NOTE : Editor 카메라는 무시한다.

    // vector에서 제거
    for(auto it = registered.begin(); it != registered.end();)
    {
        if(*it == cam)
        {
            if (registered.size() == 1)
            {
                registered.pop_back();
            }
            else
            {
                auto vback = registered.back();
                registered.back() = *it;
                *it = vback;
                registered.pop_back();
            }
            break;
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

int CameraSystem::GetCurrCameraIndex() const
{
    return currCameraIndex;
}

Camera* CameraSystem::GetCurrCamera()
{
    // NOTE : 씬 전환 뒤 currCamera Index 초과 방지
    if (currCameraIndex >= registered.size()) currCameraIndex = 0; // 초과 되면 0번 초기화

    if (registered.size() == 1)
    {
        return registered[currCameraIndex];
    }
    else
    {
        if (registered[currCameraIndex])
        {
            if (registered[currCameraIndex]->GetOwner())
            {
                if (registered[currCameraIndex]->GetOwner()->GetName() == "FreeCamera") // 현재 카메라가 프리캠이면 다음 카메라로 실행
                {
                    NextCamera(); // ++; -> 만약 여기서도 프리캠밖에 없으면 프리캠으로 잡힐 것임.
                }
            }
        }

        return registered[currCameraIndex];
    }

    return registered[currCameraIndex]; // 예외 
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

void CameraSystem::SetCurrCameraByName(std::string name)
{
    for (int i = 0; i < registered.size(); i++)
    {
        if (registered[i]->GetOwner()->GetName() == name)
        {
            currCameraIndex = i;
            return;
        }
    }

    currCameraIndex = 0; // 없으면 freeCam, mainCamera 반환
}

void CameraSystem::SetCurrCameraToFreeCamera()
{
    currCameraIndex = 0;
}

void CameraSystem::Clear()
{
    while (!registered.empty())
    {
        Camera* cam = registered.back();
        if (cam && cam->GetOwner() && cam->GetOwner()->GetName() == "FreeCamera")
        {
            registered.pop_back(); // 혹은 계속 유지할 거면 break/continue 정책을 명확히
            continue;
        }
        RemoveCamera(cam);
    }
}

void CameraSystem::LightCameraUpdate(float delta)
{
    lightCamera->Update(GetCurrCamera());
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

#include "../Components/FreeCamera.h"

Handle CameraSystem::CreateFreeCamera(int clientWidth, int clientHeight, Scene *currScene)
{
    Handle handle = ObjectSystem::Instance().Create<GameObject>();
    freeCamObj = ObjectSystem::Instance().Get<GameObject>(handle);
	freeCamObj->SetName("FreeCamera");
    freeCamera = freeCamObj->AddComponent<Camera>();
    auto freecam = freeCamObj->AddComponent<FreeCamera>();

    freeCamObj->SetActive(true); // 게임 오브젝트에 있는 모든 컴포넌트 활성화

	freeCamera->SetProjection(DirectX::XM_PIDIV2, clientWidth, clientHeight, 0.1, 3000);

    return handle;
}
