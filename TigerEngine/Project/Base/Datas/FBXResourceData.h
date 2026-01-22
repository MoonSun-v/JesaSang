#pragma once

#include "SkeletonInfo.h"
#include "Animation.h"
#include "Mesh.h"
#include "ConstantBuffer.hpp"

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
    std::string directory = "";

    std::vector<Mesh> meshes;
	std::vector<Texture> textures;	
    std::vector<Animation> animations;

    SkeletonInfo skeletalInfo;
    OffsetMatrixCB m_BoneOffsets{};

    // sub mesh matrixs
    vector<Matrix> meshes_bindMat;
    vector<Matrix> meshes_localMat;
    vector<Matrix> meshes_modelMat;

    // AABB Debug Draw
	Vector3 boxMin { FLT_MAX, FLT_MAX, FLT_MAX };
	Vector3 boxMax{};
	Vector3 boxCenter{};
};