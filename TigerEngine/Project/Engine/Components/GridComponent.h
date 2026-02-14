#pragma once
#include "pch.h"
#include "../Object/Component.h"

struct GridCell
{
    bool walkable = true;
    int x = 0;
    int y = 0;
};

// 에디터에서 지정할 좌표 목록
struct WalkableOverride
{
    int cx = 0; // 중앙 기준 X
    int cy = 0; // 중앙 기준 Y
    bool walkable = false;
};


class GridComponent : public Component
{
    RTTR_ENABLE(Component)
public:
    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

public:
    int width = 70;
    int height = 70;
    float cellSize = 80.0f;

    std::vector<GridCell> cells;
    std::vector<WalkableOverride> walkableOverrides;

    // Grid 안정화용
    bool m_pendingBuild = true;
    int  m_lastActorCount = -1;

public:
    GridComponent() = default;
    ~GridComponent() = default;

    void OnInitialize() override;

    void OnStart() override;
    void OnDestory() override;

    // register enable
    void Enable_Inner() override;
    void Disable_Inner() override;

    GridCell* GetCell(int x, int y);
    bool IsWalkable(int x, int y);

    Vector3 GridToWorld(int x, int y);
    bool WorldToGrid(const Vector3& pos, int& outX, int& outY);


    // (0,0) 을 중점으로 해서 좌표 가져오도록 하는 메소드 : 우리가 흔히 아는 좌표계 
    GridCell* GetCellFromCenter(int cx, int cy);
    bool IsWalkableFromCenter(int cx, int cy); 
    Vector3 GridToWorldFromCenter(int cx, int cy);
    bool WorldToGridFromCenter(const Vector3& pos, int& outCX, int& outCY);
    void SetWalkableFromCenter(int cx, int cy, bool walkable); 

    // Width / Height 변경 시 호출
    void ResizeGrid(int newWidth, int newHeight);

    // 씬에 있는 Trigger 아닌 Collider는 Grid 차단 
    void BuildBlockedFromPhysics();
    void BuildWalkableFromCostum();

    // [ A* ]
    std::vector<std::pair<int, int>> FindPath(int startCX, int startCY, int endCX, int endCY);
};
