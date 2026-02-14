#pragma once
#include "Components/ScriptComponent.h"

class Image;
class SearchObject_Tutorial : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

public:
    // Json
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);

    Image* image_sensorOn = nullptr;
    Image* image_interactionOn = nullptr;
    Image* image_interactionGauge = nullptr;

    bool isCompleted = false;
    float holdTime = 1.5f;
    float gauge = 0.0f;

    void OnStart() override;

    void ShowUI(bool on);
    bool UpdateInteraction(float dt); // true = ¿Ï·á
};
