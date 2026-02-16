#pragma once
#include "../../../Base/pch.h"
#include "../../../Engine/Components/ScriptComponent.h"
#include "../../../Engine/Components/AudioSourceComponent.h"

enum class EnemySoundType
{
    Ghost_Move_Sound,                   // 어른 적 이동
    Ghost_Frozen_Sound,                 // ?
    Ghost_Playerfind_Sound,             // 플레이어 찾음
    Ghost_Attack_Sound,                 // 플레이어 공격
    Ghost_AttackDelay_Sound,            // ?
    BabyGhost_Move_Sound,               // 꼬마적 이동
    BabyGhost_Playerfind_Sound,          // 꼬마, 플레이어 찾음
    BabyGhost_Crying_Sound
};

class EnemySoundSource : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)
public:
    void OnStart() override;
    void OnUpdate(float dt) override;

    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

    /// <summary>
    /// 외부에서 호출할 사운드 출력함수
    /// </summary>
    /// <param name="type">플레이어 사운드 타입</param>
    void PlaySound(EnemySoundType type, bool loop = false);
    void UpdateAudioTransform(float delta); // 3D 사운드용 오디오 트랜스폼 업데이트 함수

private:
    AudioSourceComponent* sound = nullptr;

    bool hasPrev = false;
    Vector3 prevPos{};
};
