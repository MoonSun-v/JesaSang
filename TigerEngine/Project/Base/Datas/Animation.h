#pragma once
#include <string>
#include <vector>
#include "NodeAnimation.h"
#include <map>
#include "SkeletonInfo.h"

using namespace std;

class Animation
{
public:
	map<string, int> m_mappingNodeAnimations; 
	vector<NodeAnimation> m_nodeAnimations;		 // bone index = vector index
	float  m_duration;							
	string m_name;								
	float  m_tick;					

    bool m_loop;

    void EvaluatePose( float time, const SkeletonInfo* skeleton, vector<Matrix>& outPose ) const;

    void CreateFromAssimp(aiAnimation* anim);
};