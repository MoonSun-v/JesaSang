#pragma once

#include "pch.h"

#include <string>

#include "..\\Externals\\AudioModule_FMOD\\include\\AudioController.h"

class Camera;
class Transform;

class AudioSceneController
{
public:
    void Init(const std::string& entryId);
    void Shutdown();
    void UpdateFromCamera(Camera* currCam);

private:
    void FindNearestSphereTransform(const Vector3& listenerPos);

    AudioController m_audioController{};
    Transform* m_sourceTransform = nullptr;
    std::string m_entryId{};

    DirectX::XMFLOAT3 m_listenerPos{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 m_listenerVel{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 m_listenerFwd{ 0.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT3 m_listenerUp{ 0.0f, 1.0f, 0.0f };

    DirectX::XMFLOAT3 m_sourcePos{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 m_sourceVel{ 0.0f, 0.0f, 0.0f };

    bool m_ready = false;
    bool m_started = false;
    bool m_sourceLocked = false;
};
