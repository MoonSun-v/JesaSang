#pragma once

#include <directxmath.h>
#include "..\\fmod\\inc\\fmod.hpp"
#include <memory>

class AudioClip;

class AudioSource {
public:
    void Init(FMOD::System* system);

    void SetClip(std::shared_ptr<AudioClip> clip);
    void SetLoop(bool loop);
    void SetVolume(float volume);
    void SetPitch(float pitch);
    void SetChannelGroup(FMOD::ChannelGroup* group);
    void Set3DMinMaxDistance(float minDist, float maxDist);
    void SetPosition(const DirectX::XMFLOAT3& pos);
    void SetVelocity(const DirectX::XMFLOAT3& vel);

    void Play(bool restart = true);
    void PlayOneShot();
    void Pause(bool paused);
    void Stop();
    void Update3D();
    bool IsPlaying() const;

private:
    FMOD::System* m_System = nullptr;
    std::shared_ptr<AudioClip> m_Clip;
    FMOD::Channel* m_Channel = nullptr;
    FMOD::ChannelGroup* m_Group = nullptr;

    DirectX::XMFLOAT3 m_Position{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 m_Velocity{ 0.0f, 0.0f, 0.0f };
    float m_Volume = 1.0f;
    float m_Pitch = 1.0f;
    bool m_Loop = false;
    float m_MinDistance = 1.0f;
    float m_MaxDistance = 100.0f;
};
