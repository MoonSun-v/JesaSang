#pragma once
#include "ITutorialStep.h"

// -----------------------------------------------------------
// [ Step8 ]
// 플레이어 조작 가능 + 플레이어 바구니 뒤진 후 지도조각 획득
// 
// 1. 방 밖으로 나가지만 못하고, 자유롭게 움직이면서 플레이어 직접 조작 !!! 
// 2. 바구니에 상호작용(F) 하면 -> 대사 출력 + 지도 조각 얻기 
// -----------------------------------------------------------

enum class Step8Phase
{
    FreePlay,
    Monologue,
    Done
};

class SearchObject_Tutorial;
class TutorialStep_Step8 : public ITutorialStep
{
private:
    bool isDone = false;

    float stepTimer = 0.0f;
    float delayStart = 1.0f;

    Step8Phase phase = Step8Phase::FreePlay;

    SearchObject_Tutorial* basket = nullptr;

    void FreePlay();
    void Monologue();
    void End();

public:
    TutorialStep_Step8(TutorialController* _tutorialController) : ITutorialStep(_tutorialController) {}
    ~TutorialStep_Step8() = default;

    void Enter() override;
    void Update(float deltaTime) override;
    bool IsComplete() override;
    void Exit() override;
};