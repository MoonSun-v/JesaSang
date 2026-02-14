#pragma once
#include "ITutorialStep.h"
#include "Components/UI/Image.h"

// -----------------------------------------------------------
// [ Step3 ]
// 아기 유령 입장
// 
// 1. 아기 유령이 방으로 들어옴
// 2. 플레이어 쪽을 쳐다봄
// 3. (!) 느낌표 이펙트 잠깐 등장
// 4. 아기 유령은 울면서 밖으로 퇴장
// 5. Step4 전환
// -----------------------------------------------------------

enum class Step3Phase
{
    BabyInCome, 
    TrunToPlayer,
    Surprised,
    Leave,
    Frozen,
    Done
};

class TutorialStep_Step3 : public ITutorialStep
{
private:
    bool isDone = false;

    float step3Timer = 0.0f;
    float delayStep3 = 1.0f;

    float frozenPhaseTimer = 0.0f;
    bool dialogueShown = false;
    bool secondDialogueShown = false;

    Step3Phase phase = Step3Phase::BabyInCome;

    GameObject* targetA = nullptr;
    GameObject* targetB = nullptr;
    GameObject* exclamation = nullptr;
    AnimationController* ExclEffect = nullptr;

    Transform* babyTranform = nullptr;

    bool phaseStarted = false;

    float speed = 120.0f;

    // --- Frozen Effect ---
    Image* frozenImage = nullptr;

    float len1 = 1000.0f;
    float len2 = 400.5f;
    float len3 = 300.0f;
    float len4 = 200.5f;
    float len5 = 100.0f;

    int frozenLevel = 0;
    int nextFrozenLevel = 0;

    float frozenTimer = 0.0f;
    float frozenMaxTime = 0.5f;

    std::vector<std::string> frozenPaths;

    void UpdateFrozen(float dt);

private:
    void BabyInCome();
    void TrunToPlayer();
    void Surprised();
    void Leave();
    void Frozen();

public:
    TutorialStep_Step3(TutorialController* _tutorialController) : ITutorialStep(_tutorialController) {}
    ~TutorialStep_Step3() = default;

    void Enter() override;
    void Update(float deltaTime) override;
    bool IsComplete() override;
    void Exit() override;
};