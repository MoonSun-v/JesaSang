#pragma once

#include "pch.h"
#include "../Object/Component.h"
#include "..\\..\\Externals\\AudioModule_FMOD\\include\\AudioSource.h"
#include "..\\..\\Externals\\AudioModule_FMOD\\include\\AudioTransform.h"

class AudioSystem;
class AudioClip;

class AudioSourceComponent : public Component
{
    RTTR_ENABLE(Component)
public:
    AudioSourceComponent() { SetName("AudioSource"); }
    ~AudioSourceComponent() override = default;

    void OnInitialize() override;
    void OnUpdate(float delta) override;
    void OnDestory() override;

    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

    void Init(AudioSystem* system);
    void BindTransform(const AudioTransformRef& ref);
    void SetFallback(const AudioTransform& transform);

    const std::string& GetClipId() const;
    void SetClipId(const std::string& id);

    bool GetLoop() const;
    void SetClip(std::shared_ptr<AudioClip> clip);
    void SetLoop(bool loop);
    float GetVolume() const;
    void SetVolume(float volume);
    float GetPitch() const;
    void SetPitch(float pitch);
    float GetMinDistance() const;
    float GetMaxDistance() const;
    void Set3DMinMaxDistance(float minDist, float maxDist);
    void SetMinDistance(float minDist);
    void SetMaxDistance(float maxDist);

    void Play(bool restart = true);
    void PlayOneShot();
    void Stop();
    void Update3D();
    bool IsPlaying() const;

private:
    AudioSystem* m_System = nullptr;
    AudioSource m_Source{};
    AudioTransform m_Fallback{};
    AudioTransformRef m_Bind{};

    std::string m_ClipId{};
    float m_Volume = 1.0f;
    float m_Pitch = 1.0f;
    bool m_Loop = false;
    float m_MinDistance = 1.0f;
    float m_MaxDistance = 100.0f;
};
