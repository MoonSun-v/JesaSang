#pragma once
#include "Components/ScriptComponent.h"
#include "../AdultGhostController.h"

class AdultGhostWaypoint_A : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);

public:
    void OnInitialize() override;
};

