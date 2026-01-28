#pragma once
#include "Components/ScriptComponent.h"
#include "Components/AudioSourceComponent.h"

class AudioKeyTriggerScript : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)
public:
    AudioKeyTriggerScript() { SetName("AudioKeyTriggerScript"); }
    ~AudioKeyTriggerScript() override = default;

    void OnInitialize() override;
    void OnUpdate(float delta) override;

    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

    std::string clipId = "SFX_OneShot";

private:
    AudioSourceComponent* m_Source = nullptr;
};
