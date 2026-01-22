#pragma once
#include "pch.h"
#include "Datas/Mesh.h"
#include "Datas/Bone.h"
#include "Datas/FBXResourceData.h"
#include "Datas/ConstantBuffer.hpp"

struct SkeletalRenderItem
{
    const Mesh* mesh;
    Matrix world;
    Matrix model;
    Material material;

    int refBoneIndex;
    int isSkeletal;
    int boneCount;  // skeletal bone 개수

    const PoseMatrixCB* poses;
    const OffsetMatrixCB* offsets;
};

/// <summary>
/// RenderQueue는 처리한 렌더 item들을 보관한다.
/// </summary>
class RenderQueue
{
public:
    void AddSkeletal(const SkeletalRenderItem& item)
    {
        skeletalItems.push_back(item);
    }

    const auto& GetSkeletaItems()  const { return skeletalItems; }

    /// <summary>
    /// Queue 클래스에 있는 모든 배열 초기화
    /// </summary>
    void Clear()
    {
        skeletalItems.clear();
    };

private:
    // 각 타입별로 컨테이너 추가하기
    std::vector<SkeletalRenderItem> skeletalItems;
};

