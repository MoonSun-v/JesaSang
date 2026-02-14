#pragma once
#pragma once
#include "ITutorialStep.h"

// -----------------------------------------------------------
// [ Step9 ]
// 마지막 연출만 재생하는 스텝 (플레이어 조작)
// 
// 1. 대사 출력 
// 2. [F] / [Space] / [클릭] / 방향키 중 하나의 입력 : 대화창 OFF 
// 3. 플레이어 복도로 이동 연출 
// -----------------------------------------------------------

enum class Step9Phase
{
    Monologue,
    FadeOut,
    Done
};

class TutorialStep_Step9 : public ITutorialStep
{
private:
    bool isDone = false;
    int monoIndex = 0;
    bool phaseStarted = false;

    float stepTimer = 0.0f;
    float delayStart = 1.0f;

    Step9Phase phase = Step9Phase::Monologue;

    // --- fade out ---
    float fadeTimer = 0.0f;
    float fadeMaxTime = 3.0f;
    bool fadeStarted = false;

    void Monologue();
    // void PlayerOut();
    void FadeOut(float dt);

public:
    TutorialStep_Step9(TutorialController* _tutorialController) : ITutorialStep(_tutorialController) {}
    ~TutorialStep_Step9() = default;

    void Enter() override;
    void Update(float deltaTime) override;
    bool IsComplete() override;
    void Exit() override;
};