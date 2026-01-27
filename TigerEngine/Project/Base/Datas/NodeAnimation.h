#pragma once
#include <vector>
#include <algorithm>
#include <assimp\scene.h>
#include "AnimationKey.h"

using namespace std;

// [ 키 프레임 보간 함수 ] 
template<typename KeyType, typename ValueType>
static ValueType SampleTrack(
    const std::vector<KeyType>& keys,
    float time,
    const ValueType& defaultValue)
{
    // 키가 없을 경우 기본값 반환
    if (keys.empty())
    {
        // OutputDebugStringW(L"[AnimationClip] 키가 없으므로 기본값 반환\n");
        return defaultValue;
    }

    // 키가 하나뿐일 경우 해당 키 값 반환
    if (keys.size() == 1 || time <= keys.front().Time)
    {
        // OutputDebugStringW(L"[AnimationClip] 키가 하나이므로, 해당 키 반환\n");
        return keys.front().Value;
    }

    if (time >= keys.back().Time) { return keys.back().Value; }

    // OutputDebugString((L"[AnimationClip] keys.size() : " + std::to_wstring(keys.size()) + L"\n").c_str());
    for (size_t i = 0; i + 1 < keys.size(); ++i)
    {
        const auto& k0 = keys[i];
        const auto& k1 = keys[i + 1];

        if (time >= k0.Time && time <= k1.Time)
        {
            float span = k1.Time - k0.Time;
            float t = (span > 0.0f) ? (time - k0.Time) / span : 0.0f;

            if constexpr (std::is_same_v<ValueType, Quaternion>)
            {
                Quaternion q = Quaternion::Slerp(k0.Value, k1.Value, t);
                q.Normalize();
                return q;
            }
            else
            {
                return ValueType::Lerp(k0.Value, k1.Value, t);
            }
        }
    }

    return keys.back().Value;
}


class NodeAnimation
{
public:
	string m_nodeName;				// 사용하는 본 이름

    std::vector<PositionKey> Positions;
    std::vector<RotationKey> Rotations;
    std::vector<ScaleKey>    Scales;

	void CreateKeys(aiNodeAnim* pAiNodeAnim, float perTick)
	{
        m_nodeName = pAiNodeAnim->mNodeName.C_Str();

        // Position
        for (unsigned i = 0; i < pAiNodeAnim->mNumPositionKeys; i++)
        {
            PositionKey key;
            key.Time = pAiNodeAnim->mPositionKeys[i].mTime / perTick;
            key.Value =
            {
                pAiNodeAnim->mPositionKeys[i].mValue.x,
                pAiNodeAnim->mPositionKeys[i].mValue.y,
                pAiNodeAnim->mPositionKeys[i].mValue.z
            };
            Positions.push_back(key);
        }

        // Rotation
        for (unsigned i = 0; i < pAiNodeAnim->mNumRotationKeys; i++)
        {
            RotationKey key;
            key.Time = pAiNodeAnim->mRotationKeys[i].mTime / perTick;
            key.Value =
            {
                pAiNodeAnim->mRotationKeys[i].mValue.x,
                pAiNodeAnim->mRotationKeys[i].mValue.y,
                pAiNodeAnim->mRotationKeys[i].mValue.z,
                pAiNodeAnim->mRotationKeys[i].mValue.w
            };
            Rotations.push_back(key);
        }

        // Scale
        for (unsigned i = 0; i < pAiNodeAnim->mNumScalingKeys; i++)
        {
            ScaleKey key;
            key.Time = pAiNodeAnim->mScalingKeys[i].mTime / perTick;
            key.Value =
            {
                pAiNodeAnim->mScalingKeys[i].mValue.x,
                pAiNodeAnim->mScalingKeys[i].mValue.y,
                pAiNodeAnim->mScalingKeys[i].mValue.z
            };
            Scales.push_back(key);
        }
	}

    void Evaluate(
        float time,
        Vector3& outPos,
        Quaternion& outRot,
        Vector3& outScale) const
    {
        outPos = SampleTrack<PositionKey, Vector3>(Positions, time, Vector3::Zero);
        outRot = SampleTrack<RotationKey, Quaternion>(Rotations, time, Quaternion::Identity);
        outScale = SampleTrack<ScaleKey, Vector3>(Scales, time, Vector3::One);
    }
};