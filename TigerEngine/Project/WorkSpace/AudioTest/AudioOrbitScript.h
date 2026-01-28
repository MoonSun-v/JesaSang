#pragma once
#include "Components/ScriptComponent.h"
#include "Components/AudioSourceComponent.h"

class Transform;

class AudioOrbitScript : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)
public:
    AudioOrbitScript() { SetName("AudioOrbitScript"); }
    ~AudioOrbitScript() override = default;

    void OnInitialize() override;
    void OnUpdate(float delta) override;

    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

    float radius = 300.0f;
    float speed = 1.0f;
    float height = 0.0f;

private:
    Transform* m_Transform = nullptr;
    AudioSourceComponent* m_Source = nullptr;
    Vector3 m_Center{ 0.0f, 0.0f, 0.0f };
    Vector3 m_PrevPos{ 0.0f, 0.0f, 0.0f };
    float m_Time = 0.0f;
};
