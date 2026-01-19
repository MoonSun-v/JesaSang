#pragma once

#include <directxmath.h>
#include <fmod.hpp>

class AudioListener {
public:
    void Set(const DirectX::XMFLOAT3& pos,
             const DirectX::XMFLOAT3& vel,
             const DirectX::XMFLOAT3& forward,
             const DirectX::XMFLOAT3& up);

    void Apply(FMOD::System* system) const;

private:
    DirectX::XMFLOAT3 m_Pos{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 m_Vel{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 m_Forward{ 0.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT3 m_Up{ 0.0f, 1.0f, 0.0f };
};
