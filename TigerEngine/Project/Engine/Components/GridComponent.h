#pragma once
#include "pch.h"
#include "../Object/Component.h"

#include "../Util/GridTypes.h"

#include <queue>
#include <unordered_map>
#include <functional>


// 내부 배열 인덱스 기준 셀 정보
struct GridCell
{
    bool walkable = true;
    int x = 0;
    int y = 0;
};

// 에디터/데이터에서 특정 중앙 기준 좌표의 walkable 상태를 강제 지정
struct WalkableOverride
{
    bool walkable = false;
    int cx = 0; // 중앙 기준 X
    int cy = 0; // 중앙 기준 Y
};

// 중앙 기준 좌표계용 좌표 타입 (FindPath()에서 사용)
// - (0,0)은 Grid 중앙
//struct GridCoord
//{
//    int cx = 0;
//    int cy = 0;
//};

// [ A* Node ]
// - 중앙 기준 좌표를 사용하는 길찾기 노드
struct PathNode
{
    int cx = 0;
    int cy = 0;
    float gCost = 0.0f;      // start -> current cost
    float hCost = 0.0f;      // heuristic
    PathNode* parent = nullptr;

    float fCost() const { return gCost + hCost; }
};

struct PathNodeCompare
{
    bool operator()(const PathNode* a, const PathNode* b) const
    {
        return a->fCost() > b->fCost(); // min-heap
    }
};


class AgentComponent;

// -------------------------------------------------------------
// [ GridComponent ]
// 
// - 그리드 생성/조회
// - world <-> grid 좌표 변환
// - Physic 기반 blocked cell 생성
// - A* 경로 탐색
// - 셀 점유(occupancy) 관리
// -------------------------------------------------------------

class GridComponent : public Component
{
    RTTR_ENABLE(Component)
public:
    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;


public:
    // [ Grid Settings ] // TODO : 나중에 에디터에서 조절 가능하도록
    int width = 40;
    int height = 40;
    float cellSize = 80.0f;


    // [ Grid Data ]
    std::vector<GridCell> cells;
    std::vector<WalkableOverride> walkableOverrides;


    // [ Build State ]
    // - grid rebuild이 필요한지 / physics actor 수 변화 추적
    bool m_pendingBuild = true;
    int  m_lastActorCount = -1;


    // [ Occupancy ]
    // - 중앙 기준 좌표 단위로 agent 점유 정보 저장
    std::unordered_map<int, AgentComponent*> occupiedCells;

public:
    GridComponent() = default;
    ~GridComponent() = default;

    void OnInitialize() override;
    void OnStart() override;
    void OnDestory() override;

    void Enable_Inner() override;
    void Disable_Inner() override;


public:
    // [ Cell 접근용 함수 ]
    GridCell* GetCell(int x, int y);
    bool IsWalkable(int x, int y);


    // [ (기본) Grid <-> World 좌표 변환 ] 
    // - x, y : (0,0)이 grid 좌측 하단 기준
    Vector3 GridToWorld(int x, int y);
    bool WorldToGrid(const Vector3& pos, int& outX, int& outY);


    // [ (중앙) Grid <-> World 좌표 변환 ]
    // - cx, cy : (0,0)이 grid center인 좌표계
    GridCell* GetCellFromCenter(int cx, int cy);
    bool IsWalkableFromCenter(int cx, int cy); 
    Vector3 GridToWorldFromCenter(int cx, int cy);
    bool WorldToGridFromCenter(const Vector3& pos, int& outCX, int& outCY);
    void SetWalkableFromCenter(int cx, int cy, bool walkable); 


    // [ Grid 빌드 / 업데이트 ]
    void ResizeGrid(int newWidth, int newHeight);
    void BuildBlockedFromPhysics(); // 씬에 있는 Trigger 아닌 Collider는 Grid 차단 
    void BuildWalkableFromCostum();


    // [ A* 길찾기 : 중앙 기준 ]
    // std::vector<std::pair<int, int>> FindPath(int startCX, int startCY, int endCX, int endCY);
    std::vector<GridCoord> FindPath(int startCX, int startCY, int endCX, int endCY);

    // [ Occupancy : 중앙 기준 ]
    // - 셀 점유 관리
    int MakeKey(int cx, int cy); // key로 변환
    bool IsOccupied(int cx, int cy);
    void Occupy(int cx, int cy, AgentComponent* agent);
    void Release(int cx, int cy);
    AgentComponent* GetOccupier(int cx, int cy);
};
