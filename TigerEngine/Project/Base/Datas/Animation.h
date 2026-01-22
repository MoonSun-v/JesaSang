#pragma once
#include <string>
#include <vector>
#include "BoneAnimation.h"
#include <map>

using namespace std;

class Animation
{
public:
	map<string, int> m_mappingNodeAnimations; 
	vector<NodeAnimation> m_nodeAnimations;		
	float  m_duration;							
	string m_name;								
	float  m_tick;								

	void CreateNodeAnimation(aiAnimation* pAiAnimation);
	bool GetNodeAnimationByName(string boneName, NodeAnimation& out);
};

