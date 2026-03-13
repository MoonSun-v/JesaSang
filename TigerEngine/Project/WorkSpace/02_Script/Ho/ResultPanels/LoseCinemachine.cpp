#include "LoseCinemachine.h"
#include "../../../Engine/Object/GameObject.h"
#include "../../../Engine/EngineSystem/SceneSystem.h"
#include "../../../Engine/EngineSystem/CameraSystem.h"
#include "../../../Engine/Util/ComponentAutoRegister.h"
#include "../../../Engine/Util/JsonHelper.h"
#include "../../../Base/System/InputSystem.h"
#include "LosePanel.h"
#include "../Manager/FrozenManager.h"

REGISTER_COMPONENT(LoseCinemachine);

RTTR_REGISTRATION
{
    using namespace rttr;

    registration::class_<LoseCinemachine>("LoseCinemachine")
        .constructor<>()
    (rttr::policy::ctor::as_std_shared_ptr);
}

void LoseCinemachine::OnStart()
{
    cam = SceneUtil::GetObjectByName("LoseCinemachineCamera")->GetComponent<Camera>();
}

void LoseCinemachine::OnUpdate(float dt)
{
    //if (Input::GetKeyDown(DirectX::Keyboard::V)) Play();

    if (!cam) return;
    if (notified && !isPlayed)
    {
        if (camTimer < camMaxTime)
        {
            auto camObj = cam->GetOwner();

            Vector3 dir = -cam->GetForward(); // 뒤
            float accelateValue = std::lerp(0, maxAccelspeed, camTimer / camMaxTime);
            camObj->GetTransform()->Translate(dir * cameraSpeed * accelateValue * dt); // 앞으로 이동
            camTimer += dt;
        }
        else
        {
            auto panel = SceneUtil::GetObjectByName("LosePanel")->GetComponent<LosePanel>();
            if (panel)
                panel->Play(); // 연출 플레이
        }
    }
}

void LoseCinemachine::Play()
{
    notified = true;

    // Screen UI Off
    FrozenManager* fm = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("FrozenManager")->GetComponent<FrozenManager>();
    fm->ImageActive(false);

    CameraSystem::Instance().SetCurrCameraByName(camName); // 카메라 설정
}

nlohmann::json LoseCinemachine::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void LoseCinemachine::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}