#include "WinCinemachine.h"
#include "../../../Engine/Object/GameObject.h"
#include "../../../Engine/EngineSystem/SceneSystem.h"
#include "../../../Engine/EngineSystem/CameraSystem.h"
#include "../../../Engine/Util/ComponentAutoRegister.h"
#include "../../../Engine/Util/JsonHelper.h"
#include "../../../Base/System/InputSystem.h"
#include "WinPanel.h"
#include "../../Woo/Player/PlayerController.h"
#include "../Manager/FrozenManager.h"

REGISTER_COMPONENT(WinCinemachine);

RTTR_REGISTRATION
{
    using namespace rttr;

    registration::class_<WinCinemachine>("WinCinemachine")
        .constructor<>()
    (rttr::policy::ctor::as_std_shared_ptr);
}

void WinCinemachine::OnStart()
{
    cam = SceneUtil::GetObjectByName("WinCinemachineCamera")->GetComponent<Camera>();
    spotLight = SceneUtil::GetObjectByName("WinSpotLight")->GetComponent<Light>();

    spotLight->SetActive(false);
}

void WinCinemachine::OnUpdate(float dt)
{
    //if (Input::GetKeyDown(DirectX::Keyboard::B)) Play();

    if (!cam) return;
    if (notified && !isPlayed)
    {
        if (camTimer < camMaxTime)
        {
            auto camObj = cam->GetOwner();

            Vector3 dir = cam->GetForward();
            camObj->GetTransform()->Translate(dir * cameraSpeed * dt); // 앞으로 이동
            camTimer += dt;
        }
        else
        {
            // 카메라 연출 끝 스폿 라이트 연출 시작
            spotTimer += dt;
            
            if (spotTimer >= spotMaxTime)
            {
                spotLight->SetActive(true);
                isPlayed = true;

                auto panel = SceneUtil::GetObjectByName("WinPanel")->GetComponent<WinPanel>();
                if (panel)
                    panel->Play(); // 연출 플레이
            }
        }
    }
}

void WinCinemachine::Play()
{
    notified = true;

    auto player = SceneUtil::GetObjectByName("Player")->GetComponent<PlayerController>();
    if(player) 
        player->GameSuceesPlayerStop();

    // Screen UI Off
    FrozenManager* fm = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("FrozenManager")->GetComponent<FrozenManager>();
    fm->ImageActive(false);

    CameraSystem::Instance().SetCurrCameraByName(camName); // 카메라 설정
}

nlohmann::json WinCinemachine::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void WinCinemachine::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}
