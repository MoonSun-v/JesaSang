#include "..\\include\\AudioListener.h"

void AudioListener::Set(const DirectX::XMFLOAT3& pos,
                        const DirectX::XMFLOAT3& vel,
                        const DirectX::XMFLOAT3& forward,
                        const DirectX::XMFLOAT3& up)
{
    m_Pos = pos;
    m_Vel = vel;
    m_Forward = forward;
    m_Up = up;
}

void AudioListener::Apply(FMOD::System* system) const
{
    if (!system) {
        return;
    }

    FMOD_VECTOR p{ m_Pos.x, m_Pos.y, m_Pos.z };
    FMOD_VECTOR v{ m_Vel.x, m_Vel.y, m_Vel.z };
    FMOD_VECTOR f{ m_Forward.x, m_Forward.y, m_Forward.z };
    FMOD_VECTOR u{ m_Up.x, m_Up.y, m_Up.z };
    system->set3DListenerAttributes(0, &p, &v, &f, &u);
}
