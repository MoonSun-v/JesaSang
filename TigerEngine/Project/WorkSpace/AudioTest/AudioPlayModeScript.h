#pragma once

#include "Components/AudioSourceComponent.h"
#include "EngineSystem/PlayModeSystem.h"
#include "Object/Component.h"

class AudioPlayModeScript : public Component
{
    RTTR_ENABLE(Component)
public:
    AudioPlayModeScript() { SetName("AudioPlayModeScript"); }
    ~AudioPlayModeScript() override = default;

    void OnInitialize() override;
    void OnUpdate(float delta) override;

    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

    bool GetAutoPlay() const;
    void SetAutoPlay(bool enabled);

private:
    AudioSourceComponent* m_Source = nullptr;
    bool m_AutoPlay = true;
    bool m_WasPaused = false;
    bool m_HasStarted = false;
};
