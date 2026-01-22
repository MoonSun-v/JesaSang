#include "Animation.h"
#include <stdexcept>

void Animation::CreateNodeAnimation(aiAnimation* pAiAnimation)
{
	m_name = pAiAnimation->mName.C_Str();
	m_tick = pAiAnimation->mTicksPerSecond;
	m_duration = pAiAnimation->mDuration / m_tick;

	// 본에 대한 키 애니메이션 저장
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

bool Animation::GetNodeAnimationByName(string boneName, NodeAnimation& out)
{
	auto anim = m_mappingNodeAnimations.find(boneName);
	if (anim == m_mappingNodeAnimations.end())
	{
		return false;
	}

	int index = anim->second;
	out = m_nodeAnimations[index];

	return true;
}
