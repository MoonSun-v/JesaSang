#pragma once
#include "Components/ScriptComponent.h"

class Image;
class Decal;

/*
    [ CookingZone Script Component ] <Singleton>

    Cooking Zone UI 인터랙션용으로 급하게 만듦
    
    오 추가 잘했다. 여기 기능 다른버전 함정 넣어야함
    가장 가까운 성인유령 1명만 호출함!
*/


class CookingZone : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    // singleton
    inline static CookingZone* s_instance = nullptr;

    // child UI
    Image* image_sensorOn = nullptr;
    Image* image_interactionOn = nullptr;
    Image* image_interactionGauge = nullptr;

    // ring effect
    Decal* ringEffect = nullptr;;
    float radius = 1200.0f;


public:
    // component process
    void OnInitialize() override;
    void OnStart() override;
    void OnDestory() override;

    // json
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);

    // Singleton safety: no copy / move
    CookingZone() = default;
    ~CookingZone() override = default;

    CookingZone(const CookingZone&) = delete;
    CookingZone& operator=(const CookingZone&) = delete;
    CookingZone(CookingZone&&) = delete;
    CookingZone& operator=(CookingZone&&) = delete;


public:
    // 외부 call fucns..
    static CookingZone* Instance() { return s_instance; }

    // UI
    void UISensorOnOff(bool flag);              // 플레이어 감지영역 UI
    void UIInteractionOnOff(bool flag);         // 플레이어 상호작용 영역 UI
    void UIGaugeUpate(float progress);

    // Effect
    void StartTriggerWave();        // 파장 웨이브
    void NotifyAIInRange();         // AI 찾아서 호출
};

