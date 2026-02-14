#include "TutorialStep_Step3.h"

#include "System/InputSystem.h"
#include "System/TimeSystem.h"
#include "EngineSystem/SceneSystem.h"

void TutorialStep_Step3::Enter()
{
    isDone = false;
    step3Timer = 0.0f;
    dialogueShown = false;
    secondDialogueShown = false;
    frozenPhaseTimer = 0.0f;
    phase = Step3Phase::BabyInCome;

    targetA = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Baby_Target_A");
    targetB = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Baby_Target_B");
    babyTranform = tutorialController->babyGhost_Obj->GetTransform();

    // 느낌표 
    exclamation = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("FX_Exclamation");
    if (exclamation) exclamation->SetActive(false);
    ExclEffect = exclamation->GetComponent<AnimationController>();
    
    FBXResourceManager::Instance().LoadAnimationByPath(exclamation->GetComponent<FBXData>()->GetFBXInfo(),
        "..\\Assets\\Resource\\Effect\\ani_exclamation_mark.fbx", "ExclEffect");
    auto effectExclClip = ExclEffect->FindClip("ExclEffect");
    ExclEffect->AddState(std::make_unique<AnimationState>("ExclEffect", effectExclClip, ExclEffect));
    ExclEffect->ChangeState("ExclEffect");

    if (!effectExclClip)
    {
        cout << "[Player Effect Animation] Clip not found!\n" << endl;
        return;
    }

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

    std::cout << "[Step3] Enter" << std::endl;
}

void TutorialStep_Step3::Update(float deltaTime)
{
    step3Timer += deltaTime;

    switch (phase)
    {
    case Step3Phase::BabyInCome:

        BabyInCome();
        break;


    case Step3Phase::TrunToPlayer:

        TrunToPlayer();
        break;


    case Step3Phase::Surprised:

        Surprised();
        break;

    case Step3Phase::Leave:

        Leave();
        break;

    case Step3Phase::Frozen:

        Frozen();
        break;


    case Step3Phase::Done:
        isDone = true;
        break;
    }
}

bool TutorialStep_Step3::IsComplete()
{
    return isDone;
}

void TutorialStep_Step3::Exit()
{
    // 귀신 끄기 
    tutorialController->babyGhost_Obj->GetComponent<FBXRenderer>()->SetActive(false);

    std::cout << "[Step3] Exit " << std::endl;
}


// ----------------------------------------------------------------------------

// 1. 아기 유령이 방으로 들어옴
//      - Baby_Target_A로 이동
//      - Baby_Target_A에 도착하면 멈추고 다음 단계로 이동
void TutorialStep_Step3::BabyInCome()
{
    Vector3 target = targetA->GetTransform()->GetWorldPosition();
    Vector3 pos = tutorialController->babyGhost_Obj->GetTransform()->GetWorldPosition();
    Vector3 dir = target - pos;

    if (dir.Length() < 5.0f)
    {
        phase = Step3Phase::TrunToPlayer;
        return;
    }

    dir.Normalize();

    float dt = GameTimer::Instance().UnscaledDeltaTime();

    babyTranform->Translate(dir * speed * dt);

    // 이동 방향 바라보게 회전
    float yaw = atan2f(-dir.x, -dir.z);
    babyTranform->SetRotationY(yaw);
}


// 2. 플레이어 쪽을 쳐다봄
//      - 플레이어가 있는 방향으로 회전
void TutorialStep_Step3::TrunToPlayer()
{
    Vector3 dir = tutorialController->player_Obj->GetTransform()->GetWorldPosition() - babyTranform->GetWorldPosition();
    dir.y = 0;
    dir.Normalize();

    float targetYaw = atan2f(-dir.x, -dir.z);
    float curYaw = babyTranform->GetYaw();
    float delta = targetYaw - curYaw;

    // 각도 래핑 (-pi ~ pi)
    while (delta > XM_PI)  delta -= XM_2PI;
    while (delta < -XM_PI) delta += XM_2PI;

    if (fabs(delta) < 0.05f)
    {
        babyTranform->SetRotationY(targetYaw);
        phase = Step3Phase::Surprised;
        return;
    }

    float turnSpeed = 4.0f;
    float dt = GameTimer::Instance().UnscaledDeltaTime();

    babyTranform->SetRotationY(curYaw + delta * turnSpeed * dt);
}


// 3. (!) 느낌표 오브젝트 활성화  
//      - 느낌표 애니메이션 재생(loop X) & 귀신은 살짝 움찔 
void TutorialStep_Step3::Surprised()
{
    if (!phaseStarted)
    {
        phaseStarted = true;

        if (exclamation) exclamation->SetActive(true);

        step3Timer = 0.0f;
    }

    step3Timer += GameTimer::Instance().UnscaledDeltaTime();

    if (step3Timer >= 4.0f)
    {
        if (exclamation) exclamation->SetActive(false);

        phase = Step3Phase::Leave;
        phaseStarted = false;
    }
}


// 4. 아기 유령은 울면서 밖으로 퇴장 
//      - Cry 애니메이션 재생 
//      - Baby_Target_B 로 다시 이동 
void TutorialStep_Step3::Leave()
{
    if (!phaseStarted)
    {
        phaseStarted = true;
        tutorialController->BabyGhost_animController->ChangeState("Cry");
    }

    Vector3 target = targetB->GetTransform()->GetWorldPosition();
    Vector3 pos = babyTranform->GetWorldPosition();
    Vector3 dir = target - pos;

    if (dir.Length() < 5.0f)
    {
        frozenPhaseTimer = 0.0f;  
        dialogueShown = false;     

        phase = Step3Phase::Frozen;
        phaseStarted = false;
        return;
    }

    dir.Normalize();

    float speed = 60.f;
    float dt = GameTimer::Instance().UnscaledDeltaTime();

    babyTranform->Translate(dir * speed * dt);

    // 이동 방향 바라보게 회전
    float yaw = atan2f(-dir.x, -dir.z);
    babyTranform->SetRotationY(yaw);
}


void TutorialStep_Step3::Frozen()
{
    float dt = GameTimer::Instance().UnscaledDeltaTime();

    UpdateFrozen(dt);

    frozenPhaseTimer += dt;

    // 3초 후 대사 출력
    if (!dialogueShown && frozenPhaseTimer >= 3.0f)
    {
        tutorialController->dialogue->ShowDialogueText(L"갑자기 왜 이렇게 춥지..? 뭔가 불길해..!!");
        dialogueShown = true;
    }

    if (dialogueShown && !secondDialogueShown && frozenPhaseTimer >= 5.0f)
    {
        tutorialController->dialogue->ShowDialogueText( L"일단 숨어서 상황을 지켜보자..!");
        secondDialogueShown = true;
    }

    // 대사 출력 후 3초 뒤 Done 상태로 전환
    if (dialogueShown && frozenPhaseTimer >= 6.0f)
    {
        phase = Step3Phase::Done;
    }
}


void TutorialStep_Step3::UpdateFrozen(float dt)
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