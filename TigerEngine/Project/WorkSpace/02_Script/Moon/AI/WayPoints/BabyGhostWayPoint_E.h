#pragma once
#include "Components/ScriptComponent.h"
#include "../BabyGhostController.h"

class BabyGhostWayPoint_E : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

private:
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);

public:
    void OnStart() override;
};
