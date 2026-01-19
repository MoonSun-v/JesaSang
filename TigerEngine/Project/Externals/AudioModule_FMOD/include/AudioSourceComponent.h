#pragma once

#include "AudioSource.h"
#include "AudioTransform.h"

class AudioSystem;

class AudioSourceComponent {
public:
    void Init(AudioSystem* system);
    void BindTransform(const AudioTransformRef& ref);
    void SetFallback(const AudioTransform& transform);

    void SetClip(std::shared_ptr<AudioClip> clip);
    void SetLoop(bool loop);
    void SetVolume(float volume);
    void SetPitch(float pitch);
    void Set3DMinMaxDistance(float minDist, float maxDist);

    void Play(bool restart = true);
    void PlayOneShot();
    void Stop();
    void Update3D();
    bool IsPlaying() const;

private:
    AudioSystem* m_System = nullptr;
    AudioSource m_Source;
    AudioTransform m_Fallback{};
    AudioTransformRef m_Bind{};
};
