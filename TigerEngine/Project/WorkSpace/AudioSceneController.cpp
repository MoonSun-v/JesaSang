#include "AudioSceneController.h"

#include <limits>

#include "Manager/AudioManager.h"
#include "EngineSystem/SceneSystem.h"
#include "Components/FBXData.h"
#include "Components/Camera.h"
#include "Object/GameObject.h"

void AudioSceneController::Init(const std::string& entryId)
{
    if (m_ready)
    {
        return;
    }

    m_entryId = entryId;

    auto& audioMgr = AudioManager::Instance();
    auto entry = audioMgr.GetEntry(m_entryId);
    if (!entry)
    {
        OutputDebugStringA("[AudioScene] Missing manifest entry\n");
        return;
    }

    audioMgr.GetSystem().Set3DSettings(1.0f, 1.0f, 1.0f);

    auto clip = audioMgr.GetOrCreateClip(entry->id);
    if (!clip)
    {
        OutputDebugStringA("[AudioScene] Failed to create clip\n");
        return;
    }

    m_audioController.Init(&audioMgr.GetSystem());

    AudioTransformRef listenerRef{};
    listenerRef.position = &m_listenerPos;
    listenerRef.velocity = &m_listenerVel;
    listenerRef.forward = &m_listenerFwd;
    listenerRef.up = &m_listenerUp;
    m_audioController.BindListener(listenerRef);

    AudioTransformRef sourceRef{};
    sourceRef.position = &m_sourcePos;
    sourceRef.velocity = &m_sourceVel;
    m_audioController.BindSource(sourceRef);

    m_audioController.SetClip(clip);
    m_audioController.SetLoop(entry->loop);
    m_audioController.SetVolume(entry->defaultVolume);
    m_audioController.Set3DMinMaxDistance(8.0f, 1200.0f);

    m_ready = true;
}

void AudioSceneController::Shutdown()
{
    if (!m_ready)
    {
        return;
    }

    m_audioController.Stop();
    m_audioController.SetClip({});
    m_ready = false;
    m_started = false;
    m_sourceLocked = false;
    m_sourceTransform = nullptr;
}

void AudioSceneController::UpdateFromCamera(Camera* currCam)
{
    if (!m_ready)
    {
        return;
    }

    if (currCam && currCam->GetOwner())
    {
        auto transform = currCam->GetOwner()->GetTransform();
        if (transform)
        {
            const auto& pos = transform->GetPosition();
            m_listenerPos = { pos.x, pos.y, pos.z };
        }

        auto forward = currCam->GetForward();
        m_listenerFwd = { forward.x, forward.y, forward.z };
    }

    FindNearestSphereTransform(Vector3{ m_listenerPos.x, m_listenerPos.y, m_listenerPos.z });

    if (m_sourceTransform)
    {
        const auto& pos = m_sourceTransform->GetPosition();
        m_sourcePos = { pos.x, pos.y, pos.z };
    }
    else if (!m_sourceLocked)
    {
        auto right = currCam ? currCam->GetRight() : Vector3::Right;
        m_sourcePos = {
            m_listenerPos.x - right.x * 5.0f,
            m_listenerPos.y - right.y * 5.0f,
            m_listenerPos.z - right.z * 5.0f
        };
        m_sourceLocked = true;
    }

    m_audioController.Update();

    if (!m_started)
    {
        m_audioController.Play();
        m_started = true;
    }
}

void AudioSceneController::FindNearestSphereTransform(const Vector3& listenerPos)
{
    auto scene = SceneSystem::Instance().GetCurrentScene();
    if (!scene)
    {
        return;
    }

    float bestDist = std::numeric_limits<float>::max();
    Transform* best = nullptr;
    scene->ForEachGameObject([&](GameObject* obj)
    {
        auto fbx = obj->GetComponent<FBXData>();
        if (!fbx)
        {
            return;
        }

        if (fbx->path.find("sphere.fbx") == std::string::npos)
        {
            return;
        }

        auto transform = obj->GetTransform();
        if (!transform)
        {
            return;
        }

        float dist = (transform->GetPosition() - listenerPos).Length();
        if (dist < bestDist)
        {
            bestDist = dist;
            best = transform;
        }
    });

    if (best)
    {
        m_sourceTransform = best;
    }
}
