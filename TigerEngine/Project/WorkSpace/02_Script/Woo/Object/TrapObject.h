#pragma once
#include "Components/ScriptComponent.h"
class Decal;

/*
    [ TrapObject Script Component ]

    함정 오브젝트입니다.
    트리거 범위 안에 플레이어가 들어왔을 경우 플레이어 상태에 따른 파장을 발생시키고,
    그 범위 내의 AI를 호출합니다.

    재발동 쿨타임 : 5초

    - 함정 오브젝트 종류
     1) 깨진 바닥 (Decal)
     2) 낡은 책상 (Model)
*/

class TrapObject : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

public:
    bool isTable = false;

private:
    // component
    Decal* ringEffect = nullptr;;

    // cur
    float curWaveRadius = 0.0f;
    float curWaveSpeed = 0.3f;
    float curAiRange = 0.0f;

    // data
    float walkWaveRadius = 0.45f;
    float walkWaveSpeed = 0.2f;
    float runWaveRadius = 1;
    float runWaveSpeed = 0.35f;
    float coolTime = 5.0f;

    float walkAiRange_decal = 400.0f;
    float runAiRange_decal = 800.0f;
    float walkAiRange_table = 600.0f;
    float runAiRange_table = 1000.0f;

    // rewave cool time
    bool isPossibleWave = true;
    bool isWaveing = false;
    float coolTimer = 0.0f;

public:
    // component process
    void OnStart() override;
    void OnUpdate(float delta) override;

    // json
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);

    // trigger event
    void OnCCTTriggerEnter(CharacterControllerComponent* other) override;

private:
    void StartTriggerWave();        // 파장 웨이브
    void NotifyAIInRange();         // AI 찾아서 호출

public:
    // 외부 call funcs..

};

