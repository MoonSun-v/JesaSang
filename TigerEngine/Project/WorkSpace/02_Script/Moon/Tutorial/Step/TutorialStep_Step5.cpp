#include "TutorialStep_Step5.h"

#include "System/InputSystem.h"
#include "EngineSystem/SceneSystem.h"
#include "System/TimeSystem.h"

#include "../../../Woo/Player/PlayerController.h"
#include "../../../Woo/Camera/CameraController.h"


// - 플레이어 렌더 끄고, hold상태에서 나오기 
// - 카메라와 다이얼로그 귀신에게 맞추기 
void TutorialStep_Step5::Enter()
{
    isDone = false;
    step5Timer = 0.0f;
    phase = Step5Phase::AdultInCome;
    phaseStarted = false;

    // 조작 불가 
    tutorialController->player_Obj->GetComponent<PlayerController>()->SetInputLock(true);
    
    // 플레이어를 강제로 idle 상태로 만들기
    auto playerCtrl = tutorialController->player_Obj->GetComponent<PlayerController>();
    playerCtrl->ChangeState(PlayerState::Idle);  
    // 렌더도 끄기
    tutorialController->player_Obj->GetComponent<FBXRenderer>()->SetActive(false);

    targetA = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Adult_Target_A");
    targetB = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Adult_Target_B");

    // 어른 귀신의 위치를 targetA로 세팅 
    tutorialController->adultGhost_Obj->GetTransform()->SetPosition(targetA->GetComponent<Transform>()->GetWorldPosition());
    adultTranform = tutorialController->adultGhost_Obj->GetTransform();

    // CameraController 타겟 변경 (귀신 바라봐야함)
    auto camCtrl = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("MainCamera")->GetComponent<CameraController>();
    if (camCtrl) camCtrl->SetTargetTransform(tutorialController->adultGhost_Obj->GetTransform());

    // 다이얼로그 타겟 변경 (귀신 기준)
    if (tutorialController->dialogue)
    {
        auto pointObj = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Ghost_DialoguePoint");
        auto point = pointObj->GetComponent<Transform>();
        tutorialController->dialogue->SetdialogueTarget(point);
    }

    std::cout << "[Step5] Enter" << std::endl;
}

void TutorialStep_Step5::Update(float deltaTime)
{
    step5Timer += deltaTime;

    switch (phase)
    {
    case Step5Phase::AdultInCome:
        AdultInCome();
        break;

    case Step5Phase::LookAround:
        LookAround();
        break;

    case Step5Phase::Monologue1:
        Monologue1();
        break;

    case Step5Phase::WaitInput1:
        WaitInput1();
        break;

    case Step5Phase::Monologue2:
        Monologue2();
        break;

    case Step5Phase::WaitInput2:
        WaitInput2();
        break;

    case Step5Phase::AdultOut:
        AdultOut();
        break;

    case Step5Phase::Done:
        isDone = true;
        break;
    }
}

bool TutorialStep_Step5::IsComplete()
{
    return isDone;
}

void TutorialStep_Step5::Exit()
{
    // 귀신 끄기 
    tutorialController->adultGhost_Obj->GetComponent<FBXRenderer>()->SetActive(false);

    // 카메라 타겟 플레이어로 복구
    auto camCtrl = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("MainCamera")->GetComponent<CameraController>();
    if (camCtrl) camCtrl->SetTargetTransform(tutorialController->player_Obj->GetTransform());

    // 렌더도 다시 켜기
    tutorialController->player_Obj->GetComponent<FBXRenderer>()->SetActive(true);

    std::cout << "[Step5] Exit " << std::endl;
}


// -------------------------------------------------------------

// 1. 어른 유령이 방으로 들어옴(TargetB) & 화면 외각 프로즌 점점 심해짐 
void TutorialStep_Step5::AdultInCome()
{
    // TargetB로 이동 
    Vector3 target_b = targetB->GetTransform()->GetWorldPosition();
    Vector3 pos = tutorialController->adultGhost_Obj->GetTransform()->GetWorldPosition();
    Vector3 dir = target_b - pos;

    if (dir.Length() < 5.0f)
    {
        phase = Step5Phase::LookAround;
        return;
    }

    dir.Normalize();

    float dt = GameTimer::Instance().UnscaledDeltaTime();

    adultTranform->Translate(dir * speed * dt);

    // 이동 방향 바라보게 회전
    float yaw = atan2f(-dir.x, -dir.z);
    adultTranform->SetRotationY(yaw);
}


// 2. adultGhost_Obj 좌우로 두리번 두리번 거리기 
void TutorialStep_Step5::LookAround()
{
    if (!phaseStarted)
    {
        phaseStarted = true;
        step5Timer = 0.0f;
    }

    float dt = GameTimer::Instance().UnscaledDeltaTime();

    // 좌우로 살짝 회전
    float duration = 7.0f;

    // 회전 범위 (라디안)
    float angleRange = XMConvertToRadians(50.0f); 

    // 현재 시간 비율
    float t = step5Timer / duration;

    float baseYaw = XM_PI;
    float offset = sinf(t * XM_2PI) * angleRange;

    adultTranform->SetRotationY(baseYaw + offset);

    step5Timer += dt;

    if (step5Timer >= duration)
    {
        phaseStarted = false;
        step5Timer = 0.0f;
        phase = Step5Phase::Monologue1;
    }
}

void TutorialStep_Step5::Monologue1()
{
    if (tutorialController->dialogue)
        tutorialController->dialogue->ShowDialogueHold(L"어 여기 맛있는 냄새가 나는데?");

    phase = Step5Phase::WaitInput1;
}

void TutorialStep_Step5::WaitInput1()
{
    if (
        Input::GetKeyDown(Keyboard::F) ||
        Input::GetKeyDown(Keyboard::Space) ||
        Input::GetMouseButtonDown(0)
        )
    {
        if (tutorialController->dialogue)
            tutorialController->dialogue->DialogueOnOff(false);

        phase = Step5Phase::Monologue2;
        monologuePlayed = false;
    }
}

void TutorialStep_Step5::Monologue2()
{
    if (tutorialController->dialogue)
        tutorialController->dialogue->ShowDialogueHold(L"아무것도 없네, 배고파서 헛 냄새를 맡았나..");

    phase = Step5Phase::WaitInput2;
}

void TutorialStep_Step5::WaitInput2()
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
        if (tutorialController->dialogue)
            tutorialController->dialogue->DialogueOnOff(false);

        phase = Step5Phase::AdultOut;
    }
}

// Target_Out 의 위치로 이동 
void TutorialStep_Step5::AdultOut()
{
    // Target A로 이동 
    Vector3 target_out = targetA->GetTransform()->GetWorldPosition();
    Vector3 pos = tutorialController->adultGhost_Obj->GetTransform()->GetWorldPosition();
    Vector3 dir = target_out - pos;

    if (dir.Length() < 5.0f)
    {
        phase = Step5Phase::Done;
        return;
    }

    dir.Normalize();

    float dt = GameTimer::Instance().UnscaledDeltaTime();

    adultTranform->Translate(dir * speed * dt);

    // 이동 방향 바라보게 회전
    float yaw = atan2f(-dir.x, -dir.z);
    adultTranform->SetRotationY(yaw);
}