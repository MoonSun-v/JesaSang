#include "TutorialStep_Step4.h"

#include "System/InputSystem.h"
#include "System/TimeSystem.h"

#include "Manager/WorldManager.h"

#include "EngineSystem/SceneSystem.h"
#include "../../../Woo/Player/PlayerController.h"
#include "../../../Woo/UI/MainGameUIManager.h"

void TutorialStep_Step4::Enter()
{
    isDone = false;
    step4Timer = 0.0f;
    phase = Step4Phase::ShowQuest;
    phaseStarted = false;

    hideObject = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("HideObject");
    targetA = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Adult_Target_A");
    targetB = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Adult_Target_B");

    // 귀신 위치 초기화 
    tutorialController->player_Obj->GetTransform()->SetPosition(tutorialController->InitPlayerPosition);
    adultTranform = tutorialController->player_Obj->GetTransform();

    // 플레이어 위치 초기화
    tutorialController->adultGhost_Obj->GetTransform()->SetPosition(tutorialController->InitAdultPosition);
    adultTranform = tutorialController->adultGhost_Obj->GetTransform();

    // 조작 불가 
    tutorialController->player_Obj->GetComponent<PlayerController>()->SetInputLock(true);

    // 프로즌 
    frozenImage = SceneSystem::Instance().GetCurrentScene()
        ->GetGameObjectByName("FrozenManager")
        ->GetComponent<Image>();

    frozenPaths = {
        "",
        "..\\Assets\\Resource\\Effect\\Freezing\\Freezing_effect01.png",
        "..\\Assets\\Resource\\Effect\\Freezing\\Freezing_effect02.png",
        "..\\Assets\\Resource\\Effect\\Freezing\\Freezing_effect03.png",
        "..\\Assets\\Resource\\Effect\\Freezing\\Freezing_effect04.png",
        "..\\Assets\\Resource\\Effect\\Freezing\\Freezing_effect05.png"
    };

    frozenLevel = 0;
    nextFrozenLevel = 0;
    frozenTimer = frozenMaxTime;

    if (frozenImage)
        frozenImage->SetActive(false);

    // 퀘스트 창
    if (auto ui = MainGameUIManager::Instance())
    {
        ui->QuestPannelClose(0.0f);  // 즉시 닫힘 위치로 이동
        ui->TickQuestPanel(0.0f);    // 바로 적용

        ui->UpdateQuestTitle(L"[퀘스트] 몸을 숨기자!");
        ui->UpdateQuestLable(L"서둘러서 장농 또는 항아리에 몸을 숨기세요!");
        ui->SetQuestTitleOn(true);
        ui->SetQuestLableOn(true);
        ui->SetQuestCheakboxOn(false);
        ui->SetQuestLineOn(false);

        ui->QuestPannelOpen(1.0f);  // 1초 슬라이드
    }


    std::cout << "[Step4] Enter" << std::endl;
}

void TutorialStep_Step4::Update(float deltaTime)
{
    step4Timer += deltaTime;

    if (auto ui = MainGameUIManager::Instance())
    {
        ui->TickQuestPanel(deltaTime);
    }

    switch (phase)
    {
    case Step4Phase::ShowQuest:

        if (step4Timer >= 3.0f)   // 퀘스트 창 3초 보여주기
        {
            step4Timer = 0.0f;
            if (auto ui = MainGameUIManager::Instance())
            {
                ui->QuestPannelClose(1.0f);  // 1초 동안 닫기
            }
            phase = Step4Phase::AdultInComeA;
        }
        break;
    case Step4Phase::AdultInComeA: AdultInComeA(); break;
    case Step4Phase::AdultInComeB: AdultInComeB(); break;
    case Step4Phase::AdultArrive:  AdultArrive();  break;
    case Step4Phase::Fail:
        FailUpdate(GameTimer::Instance().UnscaledDeltaTime());
        break;

    case Step4Phase::Clear:
        isDone = true;
        break;
    }
}

bool TutorialStep_Step4::IsComplete()
{
    return isDone;
}

void TutorialStep_Step4::Exit()
{
    frozenImage->SetActive(false);

    std::cout << "[Step4] Exit " << std::endl;
}



// ------------------------------------------------------------------


// 1. 조상 유령 접근 
// TODO : 퀘스트 띄우기  
void TutorialStep_Step4::AdultInComeA()
{
    UpdateFrozen(GameTimer::Instance().UnscaledDeltaTime());

    // 조작 가능 
    if (!phaseStarted)
    {
        phaseStarted = true;
        tutorialController->player_Obj->GetComponent<PlayerController>()->SetInputLock(false);
    }

    // 만약 플레이어가 Hide 상태면 바로 Clear 
    if (tutorialController->player_Obj->GetComponent<PlayerController>()->GetPlayerState() == PlayerState::Hide)
    {
        phase = Step4Phase::Clear;
        return;
    }

    // TargetA 와 어른 유령의 거리가 일정이상 가까워지면 TargetB로 이동 
    Vector3 target_a = targetA->GetTransform()->GetWorldPosition();
    Vector3 pos = tutorialController->adultGhost_Obj->GetTransform()->GetWorldPosition();
    Vector3 dir = target_a - pos; 

    if (dir.Length() < 5.0f)
    {
        phase = Step4Phase::AdultInComeB;
        return;
    }

    dir.Normalize();

    float dt = GameTimer::Instance().UnscaledDeltaTime();

    adultTranform->Translate(dir * speed * dt);

    // 이동 방향 바라보게 회전
    float yaw = atan2f(-dir.x, -dir.z);
    adultTranform->SetRotationY(yaw);
}


void TutorialStep_Step4::AdultInComeB()
{
    UpdateFrozen(GameTimer::Instance().UnscaledDeltaTime());

    // 만약 플레이어가 Hide 상태면 바로 Clear 
    if (tutorialController->player_Obj->GetComponent<PlayerController>()->GetPlayerState() == PlayerState::Hide)
    {
        phase = Step4Phase::Clear;
        return;
    }
    
    // TargetB로 이동 
    Vector3 target_b = targetB->GetTransform()->GetWorldPosition();
    Vector3 pos = tutorialController->adultGhost_Obj->GetTransform()->GetWorldPosition();
    Vector3 dir = target_b - pos;

    if (dir.Length() < 5.0f)
    {
        phase = Step4Phase::AdultArrive;
        return;
    }

    dir.Normalize();

    float dt = GameTimer::Instance().UnscaledDeltaTime();

    adultTranform->Translate(dir * speed * dt);

    // 이동 방향 바라보게 회전
    float yaw = atan2f(-dir.x, -dir.z);
    adultTranform->SetRotationY(yaw);
}


void TutorialStep_Step4::AdultArrive()
{
    UpdateFrozen(GameTimer::Instance().UnscaledDeltaTime());

    if (tutorialController->player_Obj->GetComponent<PlayerController>()->GetPlayerState() == PlayerState::Hide)
    {
        phase = Step4Phase::Clear;
        return;
    }
    else
    {
        Vector3 target = tutorialController->player_Obj->GetTransform()->GetWorldPosition();  // 플레이어 위치 
        Vector3 pos = tutorialController->adultGhost_Obj->GetTransform()->GetWorldPosition(); // 어른 유령 
        Vector3 dir = target - pos;

        if (dir.Length() < 5.0f)
        {
            phase = Step4Phase::Fail;
            failFadeTimer = 0.0f;
            failStarted = false;
            return;
        }

        dir.Normalize();

        float dt = GameTimer::Instance().UnscaledDeltaTime();

        adultTranform->Translate(dir * fast_speed * dt);

        // 이동 방향 바라보게 회전
        float yaw = atan2f(-dir.x, -dir.z);
        adultTranform->SetRotationY(yaw);
    }
}


void TutorialStep_Step4::FailUpdate(float dt)
{
    auto& postProcessData = WorldManager::Instance().postProcessData;

    if (!failStarted)
    {
        failStarted = true;

        // 플레이어 조작 잠금
        tutorialController->player_Obj->GetComponent<PlayerController>()->SetInputLock(true);
    }

    failFadeTimer += dt;

    float t = std::clamp(failFadeTimer / failFadeMaxTime, 0.0f, 1.0f);

    // 점점 어두워짐
    if (failFadeTimer <= failFadeMaxTime)
    {
        postProcessData.exposure = std::lerp(0.0f, -10.0f, t);
    }
    // 완전 암전 유지 2초
    else if (failFadeTimer <= failFadeMaxTime + 2.0f)
    {
        postProcessData.exposure = -10.0f;
    }
    // 재시작
    else
    {
        postProcessData.exposure = 0.0f; // 밝기 원복
        Enter();
    }
}


// ----------------------------------

void TutorialStep_Step4::UpdateFrozen(float dt)
{
    if (!frozenImage || !tutorialController->adultGhost_Obj) return;

    auto pPos = tutorialController->player_Obj->GetTransform()->GetWorldPosition();
    auto gPos = tutorialController->adultGhost_Obj->GetTransform()->GetWorldPosition();
    float len = Vector3::Distance(pPos, gPos);

    if (len <= len1 && len > len2) nextFrozenLevel = 1;
    else if (len <= len2 && len > len3) nextFrozenLevel = 2;
    else if (len <= len3 && len > len4) nextFrozenLevel = 3;
    else if (len <= len4 && len > len5) nextFrozenLevel = 4;
    else if (len <= len5)               nextFrozenLevel = 5;
    else                               nextFrozenLevel = 0;

    if (nextFrozenLevel > frozenLevel)
    {
        frozenLevel = nextFrozenLevel;
        frozenImage->SetActive(true);
        frozenImage->ChangeData(frozenPaths[frozenLevel]);
        frozenTimer = frozenMaxTime;
    }
    else if (nextFrozenLevel < frozenLevel)
    {
        frozenTimer -= dt;
        if (frozenTimer <= 0.0f)
        {
            frozenLevel--;
            if (frozenLevel == 0)
                frozenImage->SetActive(false);
            else
                frozenImage->ChangeData(frozenPaths[frozenLevel]);

            frozenTimer = frozenMaxTime;
        }
    }
}
