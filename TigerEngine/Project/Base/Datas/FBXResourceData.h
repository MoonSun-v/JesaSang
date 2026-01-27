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

enum class ModelType
{
    Static,
    Rigid,
    Skeletal,
    None
};

struct FBXResourceAsset
{
    // model info
    std::string directory = "";
    ModelType type = ModelType::None;

    // mesh, texture, animation
    std::vector<Mesh> meshes;
	std::vector<Texture> textures;	
    std::vector<Animation> animations;

    // skeleton
    SkeletonInfo skeletalInfo;
    OffsetMatrixCB m_BoneOffsets{};

    // sub mesh (parent-child) matrixs
    vector<Matrix> meshes_bindMat;
    vector<Matrix> meshes_localMat;
    vector<Matrix> meshes_modelMat;

    // AABB Debug Draw -> NOTE : Mesh 없는 GameObject만 사용
	Vector3 boxMin { FLT_MAX, FLT_MAX, FLT_MAX };
	Vector3 boxMax{};
	Vector3 boxCenter{};

    // [ 루트 노드의 Transform ]
    // aiScene->mRootNode->mTransformation
    Matrix rootTransform;   
};