#include "TutorialStep_Step1.h"

#include "System/InputSystem.h"
#include "EngineSystem/SceneSystem.h"
#include "Components/RectTransform.h"
#include "../WorkSpace/02_Script/Woo/Player/DialogueUI/DialogueUIController.h"
#include "../../../Woo/Camera/CameraController.h"

float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}


void TutorialStep_Step1::Enter()
{
    isDone = false;
    step1Timer = 0.0f;
    phase = Step1Phase::Blink;

    tutorialController->camController->SetViewMode(CameraController::ViewMode::Front);

    tutorialController->Blink_Top = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Blink_Top"); // rectTransform 축 -700 
    tutorialController->Blink_Bottom = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Blink_Bottom"); // rectTransform 축 700 
    if (tutorialController->Blink_Top) tutorialController->Blink_Top->SetActive(true);
    if (tutorialController->Blink_Bottom) tutorialController->Blink_Bottom->SetActive(true);

    tutorialController->dialogue = tutorialController->player_Obj->GetComponent<DialogueUIController>();
}

void TutorialStep_Step1::Update(float deltaTime)
{
    step1Timer += deltaTime;

    switch (phase)
    {
    case Step1Phase::Blink:

        Blink(); 
        if (step1Timer >= BlinkDelay + 5.0f) // 연출 끝
        {
            phase = Step1Phase::Monologue;
        }
        break;


    case Step1Phase::Monologue:
        
        Monologue(); // 플레이어 대사창 활성화 및 독백 대사 출력
        phase = Step1Phase::WaitInput; // 바로 입력 대기 상태로 전환
        break;


    case Step1Phase::WaitInput:
        // 아무 입력 받으면 넘어가기 
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
            if (tutorialController->dialogue) tutorialController->dialogue->DialogueOnOff(false);

            phase = Step1Phase::Done;
        }
        break;


    case Step1Phase::Done:
        isDone = true;
        break;
    }
}

bool TutorialStep_Step1::IsComplete()
{
    return isDone;
}

void TutorialStep_Step1::Exit()
{
    std::cout << "[Step1] Exit " << std::endl;
}


// ----------------------------------------------

// 1. 눈 깜빡이는 것 같은 연출 주기
void TutorialStep_Step1::Blink()
{
    if (!tutorialController->Blink_Top || !tutorialController->Blink_Bottom) return;

    auto* topRect = tutorialController->Blink_Top->GetComponent<RectTransform>();
    auto* botRect = tutorialController->Blink_Bottom->GetComponent<RectTransform>();
    if (!topRect || !botRect) return;

    const float BlinkDelay = 2.0f;
    if (step1Timer < BlinkDelay) return;

    float t = step1Timer - BlinkDelay;

    Vector3 topPos = topRect->GetPos();
    Vector3 botPos = botRect->GetPos();

    auto Move = [&](float localT, float duration,
        float topFrom, float topTo,
        float botFrom, float botTo)
        {
            float p = std::clamp(localT / duration, 0.0f, 1.0f);
            topPos.y = Lerp(topFrom, topTo, p);
            botPos.y = Lerp(botFrom, botTo, p);
        };

    if (t < 1.0f)
        Move(t, 1.0f, -200, -600, 200, 600);
    else if (t < 2.0f)
        Move(t - 1, 1.0f, -600, -500, 600, 500);
    else if (t < 3.0f)
        Move(t - 2, 1.0f, -500, -700, 500, 700);
    else if (t < 4.0f)
        Move(t - 3, 1.0f, -700, -600, 700, 600);
    else if (t < 5.0f)
        Move(t - 4, 1.0f, -600, -1100, 600, 1100);

    topRect->SetPos(topPos);
    botRect->SetPos(botPos);
}


// 2. 독백 출력
void TutorialStep_Step1::Monologue()
{
    if (!tutorialController->dialogue) return;

    tutorialController->dialogue->ShowDialogueHold(L"으악.. 깜빡 잠들었나.. 지금 몇 시지?");
    tutorialController->camController->SetViewMode(CameraController::ViewMode::Quarter);
}