#pragma once

#include "AudioListener.h"
#include "AudioTransform.h"

class AudioSystem;

class AudioListenerComponent {
public:
    void Init(AudioSystem* system);
    void BindTransform(const AudioTransformRef& ref);
    void SetFallback(const AudioTransform& transform);
    void Update();

private:
    AudioSystem* m_System = nullptr;
    AudioListener m_Listener;
    AudioTransform m_Fallback{};
    AudioTransformRef m_Bind{};
};
