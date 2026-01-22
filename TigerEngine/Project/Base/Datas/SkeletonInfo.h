#pragma once
#include "../pch.h"
#include "assimp/scene.h"
#include <string>
#include <vector>
#include <map>

using namespace DirectX::SimpleMath;
using namespace std;

class BoneInfo
{
public:
	string name{};				
	string parentBoneName{};
	Matrix relativeTransform{};
};

class SkeletonInfo
{
private:
	map<string, int> m_boneMappingTable;
	map<string, Matrix> m_bonesOffset;
	bool isSkeletal = false;

	void CreateBoneInfoFromNode(const aiNode* pAiNode);

public:
	vector<BoneInfo> m_bones;

	BoneInfo GetBoneInfoByIndex(int index);
	BoneInfo GetBoneInfoByName(const string& boneName);
	int GetBoneIndexByName(const string& boneName);
	Matrix GetBoneOffsetByName(const string& boneName);
	bool CreateFromAiScene(const aiScene* pAiScene);

	bool IsSkeletal() { return isSkeletal; };
};