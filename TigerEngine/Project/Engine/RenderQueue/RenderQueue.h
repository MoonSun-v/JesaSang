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
};

/// <summary>
/// RenderQueue는 처리한 렌더 item들을 보관한다.
/// </summary>
class RenderQueue
{
public:
    void AddRenderItem(const RenderItem& item)
    {
        renderItems.push_back(item);
    }

    const auto& GetRendertems()  const { return renderItems; }

    /// <summary>
    /// Queue 클래스에 있는 모든 배열 초기화
    /// </summary>
    void Clear()
    {
        renderItems.clear();
    };

private:
    // 각 타입별로 컨테이너 추가하기
    std::vector<RenderItem> renderItems;
};

