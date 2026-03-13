#pragma once
#include "../../../Base/pch.h"
#include "../../../Engine/Components/ScriptComponent.h"
#include "../../Woo/Player/PlayerController.h"
#include "../../Moon/AI/AdultGhostController.h"
#include "../../../Engine/Components/UI/Image.h"

/// <summary>
/// [프로즌 시스템] 플레이어와 어른 유령 사이의 거리에 따라서 단계별 시각효과를 주는 스크립트, 거리별로 이미지 순차적 교체함.
/// </summary>
/// <remarks>
/// 해당 매니저를 소유한 객체는 Image컴포넌트를 가지고 있어야한다.
/// </remarks>
class FrozenManager : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)
public:
    FrozenManager() { SetName("FrozenManager"); }
    ~FrozenManager() = default;

    void OnInitialize() override;
    void OnStart() override;

    void OnUpdate(float dt) override;

    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

    // 효과를 표시하는 이미지 컴포넌트
    Image* frozenVignetteImage = nullptr;

    // 이미지 패스 모음
    std::vector<std::string> effectsPath{};

    // 감지 단계 감소 타이머
    float timer = 0.0f;     
    float maxTime = 0.5f;   // 단계 감소 딜레이 시간

    // 단계별 감지 범위
    float len1 = 500.0f;
    float len2 = 400.5f;
    float len3 = 300.0f;
    float len4 = 200.5f;
    float len5 = 100.0f;

    // 현재 프로즌 단계   
    int frozenLevel = 0;        // 현재 단계
    int nextfrozenLevel = 0;    // 다음 변할 단계 -> 단계 증가 시 바로 갱신, 감소 시 타이머 0초 되면 감소,

private:
    PlayerController* pc = nullptr;                 // 플레이어
    std::vector<AdultGhostController*> enemies{};    // 어른 유령만 수집
    AdultGhostController* nearestEnemy = nullptr;

    float GetLengthFromNearestEnemy();
    void SetImageByDistance(float dt);

public:
    // 게임 연출중 OFF
    bool isGameDirecting = false;
    void ImageActive(bool flag)
    {
        if (!frozenVignetteImage) return;
        isGameDirecting = !flag;
        frozenVignetteImage->SetActive(flag);
    }
};