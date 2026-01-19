#pragma once

#include "SkeletonInfo.h"
#include "Animation.h"
#include "Mesh.h"

///// @brief Bone offset 정보 버퍼
///// skeletalInfo 본 위치 정보
//struct BoneOffsetBuffer
//{
//	Matrix boneOffset[256];
//};
//
///// @brief 애니메이션 이동이 적용된 Bone 위치 정보 버퍼
//struct BonePoseBuffer
//{
//	Matrix modelMatricies[256];
//};

struct FBXResourceAsset
{
	SkeletonInfo skeletalInfo;
	std::vector<Animation> animations;
	std::vector<Mesh> meshes;
	std::vector<Texture> textures;
	
	Vector3 boxMin { FLT_MAX, FLT_MAX, FLT_MAX };
	Vector3 boxMax{};
	Vector3 boxCenter{};

	std::string directory = "";
	BoneOffsetBuffer m_BoneOffsets{};
};