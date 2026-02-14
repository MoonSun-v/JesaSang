#include "TutorialStep_Step6.h"

#include "System/InputSystem.h"
#include "EngineSystem/SceneSystem.h"

#include "../../../Woo/Player/PlayerController.h"


void TutorialStep_Step6::Enter()
{
    isDone = false;
    stepTimer = 0.0f;
    waitingStart = true;
    phase = Step6Phase::Monologue1;

    // 조작 불가 
    tutorialController->player_Obj->GetComponent<PlayerController>()->SetInputLock(true);

    // 다이얼로그 위치 다시 플레이어로 바꾸기 
    if (tutorialController->dialogue)
    {
        auto pointObj = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("DialoguePoint");
        auto point = pointObj->GetComponent<Transform>();
        tutorialController->dialogue->SetdialogueTarget(point);
    }

    std::cout << "[Step6] Enter" << std::endl;
}

void TutorialStep_Step6::Update(float deltaTime)
{
    // 시작 딜레이
    if (waitingStart)
    {
        stepTimer += deltaTime;

        if (stepTimer < delayStart)
            return;

        waitingStart = false;
        stepTimer = 0.0f;
    }


    switch (phase)
    {
    case Step6Phase::Monologue1:
        Monologue1();
        break;

    case Step6Phase::WaitInput1:
        WaitInput1();
        break;

    case Step6Phase::Monologue2:
        Monologue2();
        break;

    case Step6Phase::WaitInput2:
        WaitInput2();
        break;

    case Step6Phase::Monologue3:
        Monologue3();
        break;

    case Step6Phase::WaitInput3:
        WaitInput3();
        break;

    case Step6Phase::Done:
        isDone = true;
        break;
    }
}

bool TutorialStep_Step6::IsComplete()
{
    return isDone;
}

void TutorialStep_Step6::Exit()
{
    std::cout << "[Step6] Exit " << std::endl;
}


// -----------------------------------------------------------

void TutorialStep_Step6::Monologue1()
{
    if (tutorialController->dialogue)
        tutorialController->dialogue->ShowDialogueHold(L"분명... 유령 이었어!");

    phase = Step6Phase::WaitInput1;
}

void TutorialStep_Step6::WaitInput1()
{
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
        phase = Step6Phase::Monologue2;
    }
}

void TutorialStep_Step6::Monologue2()
{
    if (tutorialController->dialogue)
        tutorialController->dialogue->ShowDialogueHold(L"엄청 화나보였어. 배가 고프다 했는데..");

    phase = Step6Phase::WaitInput2;
}

void TutorialStep_Step6::WaitInput2()
{
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
        phase = Step6Phase::Monologue3;
    }
}

void TutorialStep_Step6::Monologue3()
{
    if (tutorialController->dialogue)
        tutorialController->dialogue->ShowDialogueHold(L"우리가 제사를 안 지내서 밥을 못 먹은 건가?");

    phase = Step6Phase::WaitInput3;
}

void TutorialStep_Step6::WaitInput3()
{
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
        phase = Step6Phase::Done;
    }
}