#include "TutorialStep_Step8.h"

#include "System/InputSystem.h"
#include "System/TimeSystem.h"
#include "EngineSystem/SceneSystem.h"

#include "../SearchObject_Tutorial.h"
#include "../../../Woo/Player/PlayerController.h"


void TutorialStep_Step8::Enter()
{
    isDone = false;
    stepTimer = 0.0f;
    phase = Step8Phase::FreePlay;

    tutorialController->player_Obj->GetComponent<PlayerController>()->SetInputLock(false);

    auto obj = SceneSystem::Instance().GetCurrentScene() ->GetGameObjectByName("SearchObject_Tutorial");

    basket = obj->GetComponent<SearchObject_Tutorial>();

    std::cout << "[Step8] Enter" << std::endl;
}

void TutorialStep_Step8::Update(float deltaTime)
{
    stepTimer += deltaTime;

    switch (phase)
    {
    case Step8Phase::FreePlay:

        if (stepTimer >= delayStart)
        {
            FreePlay();
        }
        break;


    case Step8Phase::Monologue:
        Monologue();
        break;


    case Step8Phase::Done:
        isDone = true;
        break;
    }
}

bool TutorialStep_Step8::IsComplete()
{
    return isDone;
}

void TutorialStep_Step8::Exit()
{
    std::cout << "[Step8] Exit " << std::endl;
}


// -----------------------------------------------------


void TutorialStep_Step8::FreePlay()
{
    //      FŰ   ȣ ۿ     ؼ   Ϸ  ϸ , Monologue    Ѿ   
    if (!basket || basket->isCompleted)
        return;

    //  Ÿ  üũ
    Vector3 playerPos = tutorialController->player_Obj->GetTransform()->GetWorldPosition();
    Vector3 basketPos = basket->GetOwner()->GetTransform()->GetWorldPosition();

    float dist = (playerPos - basketPos).Length();

    if (dist < 120.0f)
    {
        basket->ShowUI(true);

        if (basket->UpdateInteraction(GameTimer::Instance().DeltaTime()))
        {
            phase = Step8Phase::Monologue;
        }
    }
    else
    {
        basket->ShowUI(false);
    }
}


void TutorialStep_Step8::Monologue()
{
    static const wchar_t* line = L"     ٱ  Ͽ             ?";

    tutorialController->dialogue->ShowDialogueHold(line);

    if (Input::GetKeyDown(Keyboard::F) ||
        Input::GetKeyDown(Keyboard::Space) ||
        Input::GetMouseButtonDown(0) ||
        Input::GetKeyDown(Keyboard::Up) ||
        Input::GetKeyDown(Keyboard::Down) ||
        Input::GetKeyDown(Keyboard::Left) ||
        Input::GetKeyDown(Keyboard::Right)
        )
    {
        tutorialController->dialogue->DialogueOnOff(false);
        phase = Step8Phase::Done;
    }
}