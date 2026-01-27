#include "Animation.h"
#include <stdexcept>

void Animation::CreateFromAssimp(aiAnimation* pAiAnimation)
{
    m_name = pAiAnimation->mName.C_Str();
    m_tick = pAiAnimation->mTicksPerSecond;
    if (m_tick == 0)
        m_tick = 25.0f;

    m_duration = pAiAnimation->mDuration / m_tick;

    for (int i = 0; i < pAiAnimation->mNumChannels; i++)
    {
        aiNodeAnim* pAiNodeAnim = pAiAnimation->mChannels[i];

        NodeAnimation nodeAnim;
        nodeAnim.m_nodeName = pAiNodeAnim->mNodeName.C_Str();
        nodeAnim.CreateKeys(pAiNodeAnim, m_tick);

        m_nodeAnimations.push_back(nodeAnim);
        m_mappingNodeAnimations.insert({ nodeAnim.m_nodeName, i });
    }
}

void Animation::EvaluatePose( float time, const SkeletonInfo* skeleton, vector<Matrix>& outPose) const
{
    int count = skeleton->GetBoneCount();
    outPose.resize(count);

    vector<Matrix> localPose(count);

    // 1. local pose 생성
    for (int i = 0; i < count; i++)
    {
        const auto& bone = skeleton->m_bones[i];
        auto it = m_mappingNodeAnimations.find(bone.name);

        if (it != m_mappingNodeAnimations.end())
        {
            const NodeAnimation& nodeAnim = m_nodeAnimations[it->second];

            Vector3 pos;
            Quaternion rot;
            Vector3 scale;

            nodeAnim.Evaluate(time, pos, rot, scale);

            localPose[i] =
                Matrix::CreateScale(scale) *
                Matrix::CreateFromQuaternion(rot) *
                Matrix::CreateTranslation(pos);
        }
        else
        {
            localPose[i] = bone.localBind; // bind
        }
    }

    // 2. 부모 누적
    for (int i = 0; i < count; i++)
    {
        const auto& bone = skeleton->m_bones[i];
        if (bone.parentBoneName.empty())
            outPose[i] = localPose[i];
        else
        {
            int parentIndex = skeleton->GetBoneIndexByName(bone.parentBoneName);
            outPose[i] = localPose[i] * outPose[parentIndex];
        }
    }
}
