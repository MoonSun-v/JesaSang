#pragma once

#include <directxmath.h>

struct AudioTransform {
    DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 forward{ 0.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
};

struct AudioTransformRef {
    const DirectX::XMFLOAT3* position = nullptr;
    const DirectX::XMFLOAT3* velocity = nullptr;
    const DirectX::XMFLOAT3* forward = nullptr;
    const DirectX::XMFLOAT3* up = nullptr;
};

inline AudioTransform ResolveAudioTransform(const AudioTransform& fallback, const AudioTransformRef& ref)
{
    AudioTransform out = fallback;
    if (ref.position) {
        out.position = *ref.position;
    }
    if (ref.velocity) {
        out.velocity = *ref.velocity;
    }
    if (ref.forward) {
        out.forward = *ref.forward;
    }
    if (ref.up) {
        out.up = *ref.up;
    }
    return out;
}
