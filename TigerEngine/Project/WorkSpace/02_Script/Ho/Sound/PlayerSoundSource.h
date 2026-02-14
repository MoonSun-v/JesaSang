#pragma once
#include "../../../Base/pch.h"
#include "../../../Engine/Components/ScriptComponent.h"
#include "../../../Engine/Components/AudioSourceComponent.h"

// NOTE TODO : 반드시 switch문의 ID값이 csv id값과 동일해야함.


enum class PlayerSoundType
{
    Sit,            // 플레이어 앉기
    Walk,           // 플레이어 걷기
    Run,            // 플레이어 뛰기
    Hit,            // 플레이어 피격
    Confused,       // 플레이어 혼란
    HpDown,         // 플레이어 체력 감소
    Typing          // 타이핑
};

enum class PlayerFootStepType
{
    Player_Walk_Sound_Right,
    Player_Walk_Sound_Left,
};

enum class FootStepLoopMode
{
    SlowWalk,
    Walk,
    Run
};

class PlayerSoundSource : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)
public:
    void OnStart() override;
    void OnUpdate(float delta) override;

    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

    /// <summary>
    /// 외부에서 호출할 사운드 출력함수
    /// </summary>
    /// <param name="type">플레이어 사운드 타입</param>
    void PlaySound(PlayerSoundType type, bool loop = false);
    void StopSound();

    /// <summary>
    /// PlayerFootStepType 에서 랜덤으로 소리 재생하는 함수
    /// </summary>
    void PlayRandomFootStep();

    // 발소리 유틸 추가
    void PlayFootStepLoop(FootStepLoopMode mode);
    void StopFootStep();

private:
    AudioSourceComponent* sound = nullptr;
    AudioSourceComponent* footSound = nullptr;

    // 발소리 internals
    bool footLoopPlaying = false;
    bool nextStepRight = true;

    float stepTimer = 0.0f;
    float stepInterval = 0.3f;

    float GetIntervalByMode(FootStepLoopMode mode) const;
    void PlayOneFootStep();
};
