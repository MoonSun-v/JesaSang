#include "TutorialStep_Step2.h"

#include "System/InputSystem.h"
#include "EngineSystem/SceneSystem.h"

void TutorialStep_Step2::Enter()
{
    isDone = false;
    step2Timer = 0.0f;
    soundPlayed = false;
    phase = Step2Phase::Sound;
}


void TutorialStep_Step2::Update(float deltaTime)
{
    step2Timer += deltaTime;

    switch (phase)
    {
    case Step2Phase::Sound:

        if (!soundPlayed)
        {
            // TODO: 쿵 사운드 재생 
            // TODO: 캐릭터 머리 위 놀람 오브젝트 활성화 
            soundPlayed = true;   
        }

        if (step2Timer >= SoundDelay)
        {
            phase = Step2Phase::Monologue;
        }
        break;


    case Step2Phase::Monologue:

        Monologue();
        phase = Step2Phase::WaitInput;
        break;


    case Step2Phase::WaitInput:

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
            phase = Step2Phase::Done;
        }
        break;


    case Step2Phase::Done:
        isDone = true;
        break;
    }
}

bool TutorialStep_Step2::IsComplete()
{
    return isDone;
}

void TutorialStep_Step2::Exit()
{
    std::cout << "[Step2] Exit " << std::endl;
}


// -------------------------------------------------------

void TutorialStep_Step2::Monologue()
{
    if (!tutorialController->dialogue) return;

    tutorialController->dialogue->ShowDialogueHold(L"..!! 엄마가 왔나??");
}