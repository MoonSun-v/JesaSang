#include "TutorialStep_Step7.h"

#include "System/InputSystem.h"
#include "System/TimeSystem.h"
#include "EngineSystem/SceneSystem.h"
#include "../../../Woo/Player/PlayerController.h"
#include "../../../Woo/Camera/CameraController.h"

Vector3 LerpVec(const Vector3& a, const Vector3& b, float t)
{
    if (t > 1.0f) t = 1.0f;
    if (t < 0.0f) t = 0.0f;
    return a + (b - a) * t;
}

void TutorialStep_Step7::Enter()
{
    isDone = false;
    stepTimer = 0.0f;
    monoIndex = 0;
    phaseStarted = false;
    phase = Step7Phase::PlayerToJesasang;

    JesaSang = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("JesaSang_Tutorial");
    fruitBasket = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("SearchObject_Tutorial");
    playerTr = tutorialController->player_Obj->GetTransform();

    // MainCamera : 컴포넌트로 CameraController가지고 있음 
    auto camObj = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("MainCamera");
    camCtrl = camObj->GetComponent<CameraController>();

    tutorialController->player_Obj->GetComponent<PlayerController>()->SetInputLock(true);


    std::cout << "[Step7] Enter" << std::endl;
}

void TutorialStep_Step7::Update(float deltaTime)
{
    switch (phase)
    {
    case Step7Phase::PlayerToJesasang:
        PlayerToJesasang();
        break;

    case Step7Phase::CameraToJesasang:
        CameraToJesasang();
        break;

    case Step7Phase::Monologue:
        Monologue();
        break;

    case Step7Phase::CameraToFruit:
        CameraToFruit();
        break;

    case Step7Phase::Done:
        isDone = true;
        break;
    }
}

bool TutorialStep_Step7::IsComplete()
{
    return isDone;
}

void TutorialStep_Step7::Exit()
{
    std::cout << "[Step7] Exit " << std::endl;
}


// --------------------------------------------------------------------

// 플레이어를 제사상 앞으로 이동 시키기 (CCT)
void TutorialStep_Step7::PlayerToJesasang() 
{
    //Vector3 target = JesaSang->GetTransform()->GetWorldPosition();
    //Vector3 pos = playerTr->GetWorldPosition();
    //Vector3 dir = target - pos;

    //if (dir.Length() < 10.0f)
    //{
    //    playerCtrl->ClearForcedMove();
    //    phase = Step7Phase::CameraToJesasang;
    //    phaseStarted = false;
    //    return;
    //}

    //dir.Normalize();
    //playerCtrl->SetForcedMove(dir, 200.0f);

    phase = Step7Phase::CameraToJesasang;
}

// 카메라는 제사상 오브젝트를 비추면서 줌인 했다가 돌아와서 다시 플레이어 비추기.
void TutorialStep_Step7::CameraToJesasang()
{
    if (!phaseStarted)
    {
        phaseStarted = true;
        stepTimer = 0.0f;

        auto camStart = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("StartCam_Jesa")->GetTransform();
        auto camEnd = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("EndCam_Jesa")->GetTransform();

        auto look = JesaSang->GetTransform();

        camCtrl->PlayCinematic(
            camStart,
            camEnd,
            look,
            1.5f,   // 이동
            3.0f    // 멈춤
        );
    }

    stepTimer += GameTimer::Instance().UnscaledDeltaTime();

    if (stepTimer >= 4.0f) // 이동 + 홀드
    {
        phaseStarted = false;
        phase = Step7Phase::Monologue;
    }
}


void TutorialStep_Step7::Monologue()
{
    static const wchar_t* lines[] =
    {
        L"이대로 두면 유령이 날 잡아먹을 지도 몰라",
        L"빨리 음식부터 채워놔야겠어",
        L"우선 과일부터 놓자. 사과랑 배가 필요해~",
        L"과일 바구니가 저기 구석에 있는걸 봤는데.."
    };

    if (tutorialController->dialogue)
        tutorialController->dialogue->ShowDialogueHold(lines[monoIndex]);

    if (
        Input::GetKeyDown(Keyboard::F) ||
        Input::GetKeyDown(Keyboard::Space) ||
        Input::GetMouseButtonDown(0) ||
        Input::GetKeyDown(Keyboard::Up) ||
        Input::GetKeyDown(Keyboard::Down) ||
        Input::GetKeyDown(Keyboard::Left) ||
        Input::GetKeyDown(Keyboard::Right)
        )
    {
        monoIndex++;

        if (tutorialController->dialogue)
            tutorialController->dialogue->DialogueOnOff(false);

        if (monoIndex >= 4)
        {
            phase = Step7Phase::CameraToFruit;
            phaseStarted = false;
        }
    }
}

// 카메라가 과일바구니 오브젝트를 비추면서 줌인 했다가 돌아와서 다시 플레이어 비추기. 
// 그리고나서 2초 뒤 Step8로 전환 
void TutorialStep_Step7::CameraToFruit()
{
    if (!phaseStarted)
    {
        phaseStarted = true;
        stepTimer = 0.0f;

        camCtrl->PlayCinematic(
            SceneSystem::Instance().GetCurrentScene() ->GetGameObjectByName("StartCam_Fruit")->GetTransform(),
            SceneSystem::Instance().GetCurrentScene() ->GetGameObjectByName("EndCam_Fruit")->GetTransform(),
            fruitBasket->GetTransform(),
            1.5f,
            3.0f
        );
    }

    stepTimer += GameTimer::Instance().UnscaledDeltaTime();

    if (stepTimer >= 4.0f)
    {
        camCtrl->SetViewMode(CameraController::ViewMode::Quarter);

        phase = Step7Phase::Done;
    }
}