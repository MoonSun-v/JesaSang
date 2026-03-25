#pragma once
#include "Components/ScriptComponent.h"

/*
    [ GamePhaseManager Script Component ] <Singleton>

    제사상에 음식 4개가 올라왔을때 시네마틱을 출력하고,
    게임의 레벨이 상승되며 그래픽을 음산하게 변경합니다.
*/

class GamePhaseManager : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    // singleton
    inline static GamePhaseManager* s_instance = nullptr;

public:
    // component process
    void OnInitialize() override;
    void OnStart() override;
    void OnUpdate(float dt) override;
    void OnDestory() override;

    // json
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);

    // Singleton safety: no copy / move
    GamePhaseManager() = default;
    ~GamePhaseManager() override = default;

    GamePhaseManager(const GamePhaseManager&) = delete;
    GamePhaseManager& operator=(const GamePhaseManager&) = delete;
    GamePhaseManager(GamePhaseManager&&) = delete;
    GamePhaseManager& operator=(GamePhaseManager&&) = delete;

public:
    // 외부 call fucns..
    static GamePhaseManager* Instance() { return s_instance; }
};

