#pragma once
#include "pch.h"
#include "Datas/Mesh.h"
#include "Datas/Bone.h"
#include "Datas/FBXResourceData.h"
#include "Datas/ConstantBuffer.hpp"

class GameObject; // 전방선언

struct RenderItem
{
    // model
    ModelType modelType;
    const Mesh* mesh;
    Material material;
    
    // matrix
    Matrix world;       // static, rigid, skeletal
    Matrix model;       // rigid

    // skeleton
    int boneCount;
    int refBoneIndex;
    const PoseMatrixCB* poses;
    const OffsetMatrixCB* offsets;

    GameObject* objPtr = nullptr; // 해당 렌더 아이템의 게임 오브젝트 포인터
    // ground
    bool isGround;
};

/// <summary>
/// RenderQueue는 처리한 렌더 item들을 보관한다.
/// </summary>
class RenderQueue
{
private:
    // Render Queue
    std::vector<RenderItem> opaqueQueue;        // 불투명 오브젝트 -> Deffered Rendering
    std::vector<RenderItem> transparentQueue;   // 투명 오브젝트   -> Forward Rendering

public:
    void AddOpaqueQueue(const RenderItem& item)
    {
        opaqueQueue.push_back(item);
    }

    void AddTransparentQueue(const RenderItem& item)
    {
        transparentQueue.push_back(item);
    }

    const auto& GetOpaqueQueue()  const { return opaqueQueue; }
    const auto& GetTransparentQueue()  const { return transparentQueue; }

    void Clear()
    {
        opaqueQueue.clear();
        transparentQueue.clear();
    };
};

