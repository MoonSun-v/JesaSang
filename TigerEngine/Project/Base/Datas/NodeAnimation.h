#pragma once
#include <vector>
#include <algorithm>
#include <assimp\scene.h>
#include "AnimationKey.h"

using namespace std;

// --------- 보간 Util ----------
template <typename TKey, typename TValue>
TValue EvaluateChannel(const std::vector<TKey>& keys, float time)
{
    if (keys.empty())
        return TValue();

    if (keys.size() == 1)
        return keys[0].Value;

    size_t index = 0;
    while (index + 1 < keys.size() && time >= keys[index + 1].Time)
        index++;

    if (index + 1 >= keys.size())
        return keys.back().Value;

    const auto& A = keys[index];
    const auto& B = keys[index + 1];

    float duration = B.Time - A.Time;
    if (duration <= 0.0f)
        return A.Value;

    float t = (time - A.Time) / duration;
    t = std::clamp(t, 0.0f, 1.0f);

    if constexpr (std::is_same_v<TValue, Quaternion>)
        return Quaternion::Slerp(A.Value, B.Value, t);
    else
        return Vector3::Lerp(A.Value, B.Value, t);
}


class NodeAnimation
{
public:
	string m_nodeName;				// 사용하는 본 이름
	// vector<AnimationKey> m_keys;	// 채널(mChaneels)에 저장되어 있는 키 값들
	// string interpolationType		

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

    void Evaluate(float time,
        Vector3& outPosition,
        Quaternion& outRotation,
        Vector3& outScale) const
    {
        outPosition = EvaluateChannel<PositionKey, Vector3>(Positions, time);
        outRotation = EvaluateChannel<RotationKey, Quaternion>(Rotations, time);
        outScale = EvaluateChannel<ScaleKey, Vector3>(Scales, time);
    }

};