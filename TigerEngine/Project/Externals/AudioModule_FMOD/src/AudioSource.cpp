#include "..\\include\\AudioSource.h"
#include "..\\include\\AudioClip.h"

void AudioSource::Init(FMOD::System* system)
{
    m_System = system;
}

void AudioSource::SetClip(std::shared_ptr<AudioClip> clip)
{
    m_Clip = std::move(clip);
}

void AudioSource::SetLoop(bool loop)
{
    m_Loop = loop;
}

void AudioSource::SetVolume(float volume)
{
    m_Volume = volume;
    if (m_Channel) {
        m_Channel->setVolume(volume);
    }
}

void AudioSource::SetPitch(float pitch)
{
    m_Pitch = pitch;
    if (m_Channel) {
        m_Channel->setPitch(pitch);
    }
}

void AudioSource::Set3DMinMaxDistance(float minDist, float maxDist)
{
    m_MinDistance = minDist;
    m_MaxDistance = maxDist;
    if (m_Channel) {
        m_Channel->set3DMinMaxDistance(minDist, maxDist);
    }
}

void AudioSource::SetPosition(const DirectX::XMFLOAT3& pos)
{
    m_Position = pos;
}

void AudioSource::SetVelocity(const DirectX::XMFLOAT3& vel)
{
    m_Velocity = vel;
}

void AudioSource::Play(bool restart)
{
    if (!m_System || !m_Clip || !m_Clip->IsValid()) {
        return;
    }

    if (m_Channel) {
        bool playing = false;
        m_Channel->isPlaying(&playing);
        if (playing && !restart) {
            return;
        }
        m_Channel->stop();
        m_Channel = nullptr;
    }

    FMOD_MODE mode = 0;
    m_Clip->GetSound()->getMode(&mode);
    mode &= ~(FMOD_LOOP_NORMAL | FMOD_LOOP_OFF);
    mode |= (m_Loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
    m_Clip->GetSound()->setMode(mode);

    m_System->playSound(m_Clip->GetSound(), nullptr, false, &m_Channel);
    if (m_Channel) {
        m_Channel->setVolume(m_Volume);
        m_Channel->setPitch(m_Pitch);
        m_Channel->set3DMinMaxDistance(m_MinDistance, m_MaxDistance);
        Update3D();
    }
}

void AudioSource::PlayOneShot()
{
    if (!m_System || !m_Clip || !m_Clip->IsValid()) {
        return;
    }

    if (m_Channel) {
        m_Channel->stop();
        m_Channel = nullptr;
    }

    FMOD_MODE mode = 0;
    m_Clip->GetSound()->getMode(&mode);
    mode &= ~(FMOD_LOOP_NORMAL | FMOD_LOOP_OFF);
    mode |= FMOD_LOOP_OFF;
    m_Clip->GetSound()->setMode(mode);
    m_System->playSound(m_Clip->GetSound(), nullptr, false, &m_Channel);
    if (m_Channel) {
        m_Channel->setVolume(m_Volume);
        m_Channel->setPitch(m_Pitch);
        m_Channel->set3DMinMaxDistance(m_MinDistance, m_MaxDistance);
        Update3D();
    }
}

void AudioSource::Pause(bool paused)
{
    if (m_Channel) {
        m_Channel->setPaused(paused);
    }
}

void AudioSource::Stop()
{
    if (m_Channel) {
        m_Channel->stop();
        m_Channel = nullptr;
    }
}

void AudioSource::Update3D()
{
    if (!m_Channel) {
        return;
    }
    FMOD_VECTOR p{ m_Position.x, m_Position.y, m_Position.z };
    FMOD_VECTOR v{ m_Velocity.x, m_Velocity.y, m_Velocity.z };
    m_Channel->set3DAttributes(&p, &v);
}

bool AudioSource::IsPlaying() const
{
    bool playing = false;
    if (m_Channel) {
        m_Channel->isPlaying(&playing);
    }
    return playing;
}
