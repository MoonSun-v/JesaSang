#pragma once
#include "../pch.h"
#include <string>
#include "NodeAnimation.h"

using namespace std;

class Bone
{
public:
	void CreateBone(string objName, int parentIndex, int boneIndex, Matrix worldMat, Matrix localMat);

	NodeAnimation m_nodeAnimation;

	Matrix m_worldTransform;
	Matrix m_localTransform;

	string name = "";		

	int m_parentIndex = -1;
	int m_index = -1;		
};