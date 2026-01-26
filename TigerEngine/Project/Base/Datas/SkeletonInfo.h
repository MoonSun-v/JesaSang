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

    Matrix localBind;   // aiNode->mTransformation
    Matrix globalBind;  // 누적된 결과
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
	int GetBoneIndexByName(const string& boneName) const;
	Matrix GetBoneOffsetByName(const string& boneName);
	bool CreateFromAiScene(const aiScene* pAiScene);

    void BuildGlobalBind();
public:
    int GetBoneCount() const
    {
        return (int)m_bones.size();
    }

    Matrix GetBindPose(int index) const
    {
        if (index < 0 || index >= m_bones.size())
            return Matrix::Identity;

        return m_bones[index].globalBind;
    }

	bool IsSkeletal() { return isSkeletal; };
};