#include "GridComponent.h"
#include "../EngineSystem/GridSystem.h"
#include "../EngineSystem/PhysicsSystem.h"
#include "../Components/PhysicsComponent.h"
#include "../Components/Transform.h"
#include "../Object/GameObject.h"
#include "../Util/JsonHelper.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<GridComponent>("GridComponent")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

nlohmann::json GridComponent::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void GridComponent::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

// [ Debug Helper ] Physics → Grid 매핑 디버그 출력
namespace
{
    void DebugPrintBlockedCell(
        const std::string& objectName,
        int gridX, int gridY,
        int centerX, int centerY,
        const Vector3& worldPos,
        ColliderType type)
    {
        std::cout
            << "[" << objectName << "]  "
            << "Grid(" << gridX << "," << gridY << ")  "
            << "Center(" << centerX << "," << centerY << ")  "
            << "World(" << worldPos.x << "," << worldPos.z << ")  "
            << "Type(" << static_cast<int>(type) << ")\n";
    }
}


void GridComponent::OnInitialize()
{
    // Grid 셀 배열 초기화
    ResizeGrid(width, height);

    // Physics 상태를 보고 나중에 다시 build 할 수 있도록 플래그 초기화
    m_pendingBuild = true;
    m_lastActorCount = -1;
     
    // 1. 임의로 그리드를 걸을 수 없게 설정 : false 
    // SetWalkableFromCenter(0, 18, false);

    // 2. 임의로 그리드를 걸을 수 있도록 설정 : true 
    SetWalkableFromCenter(-8, -13, true);
    SetWalkableFromCenter(-9, -13, true);
}

void GridComponent::Enable_Inner()
{
    GridSystem::Instance().Register(this);
    OnEnable();
}

void GridComponent::OnStart()
{
}

void GridComponent::OnDestory()
{    
}

void GridComponent::Disable_Inner()
{
    GridSystem::Instance().UnRegister(this);
    OnDisable();
}

// -------------------------------------------------------------


// [ Grid Build ] 
// - Physics Actor 기반으로 Grid 차단 정보 생성
void GridComponent::BuildBlockedFromPhysics()
{
    // 1. 우선 모든 셀을 walkable 상태로 초기화
    for (auto& c : cells)
        c.walkable = true;

    auto& actorMap = PhysicsSystem::Instance().m_ActorMap;

    std::cout << "\n==== Physics → Grid Mapping (AABB based, Center Origin) ====\n";

    const int centerGridX = (width - 1) / 2;
    const int centerGridY = (height - 1) / 2;

    // 같은 GameObject가 여러 PhysicsComponent를 갖는 경우 중복 처리 방지
    std::unordered_set<GameObject*> visitedOwners;

    for (auto& pair : actorMap)
    {
        PhysicsComponent* phys = pair.first;
        if (!phys || !phys->m_Actor) continue;
        if (phys->IsTrigger()) continue;
        if (phys->GetLayer() & CollisionLayer::Ground) continue;

        GameObject* owner = phys->GetOwner();
        if (!owner) continue;

        if (visitedOwners.count(owner)) 
            continue;
        visitedOwners.insert(owner);

        Transform* transform = phys->transform;
        if (!transform)
            continue;

        Vector3 objectPos = transform->GetLocalPosition();

        std::string objName = owner ? owner->GetName() : "Unknown";

        // ----------------------------------------------------
        // 1. Collider world bounds 계산
        // ----------------------------------------------------

        Vector3 minWorld;
        Vector3 maxWorld;

        if (phys->m_ColliderType == ColliderType::Box)
        {
            PxBounds3 bounds = phys->m_Actor->getWorldBounds();

            std::cout << "\n[" << objName << "] PxBounds\n";
            std::cout << "  min = (" << bounds.minimum.x * 100 << ", " << bounds.minimum.y * 100 << ", " << bounds.minimum.z * 100 << ")\n";
            std::cout << "  max = (" << bounds.maximum.x * 100 << ", " << bounds.maximum.y * 100 << ", " << bounds.maximum.z * 100 << ")\n";
            std::cout << "  TransformPos = (" << objectPos.x << ", " << objectPos.y << ", " << objectPos.z << ")\n";

            minWorld = { bounds.minimum.x * 100.0f, 0.0f, bounds.minimum.z * 100.0f };
            maxWorld = { bounds.maximum.x * 100.0f, 0.0f, bounds.maximum.z * 100.0f };
        }
        else
        {
            float radius = 0.0f;

            switch (phys->m_ColliderType)
            {
            case ColliderType::Sphere:
                radius = phys->m_Radius;
                break;
            case ColliderType::Capsule:
                radius = phys->m_Radius;
                break;
            default:
                radius = phys->m_HalfExtents.x;
                break;
            }

            minWorld = { objectPos.x - radius, 0.0f, objectPos.z - radius };
            maxWorld = { objectPos.x + radius, 0.0f, objectPos.z + radius };
        }

        // ----------------------------------------------------
        // 2. World bounds -> 중앙 기준 grid bounds
        // ----------------------------------------------------
        int minCX = 0, minCY = 0;
        int maxCX = 0, maxCY = 0;

        WorldToGridFromCenter(minWorld, minCX, minCY);
        WorldToGridFromCenter(maxWorld, maxCX, maxCY);

        std::cout << "\n[" << objName << "] CenterGridRange: ("
            << minCX << "," << minCY << ") ~ (" << maxCX << "," << maxCY << ")\n";


        // ----------------------------------------------------
        // 3. center-based -> internal grid index + clamp
        // ----------------------------------------------------
        int minX = std::clamp(centerGridX + minCX, 0, width - 1);
        int minY = std::clamp(centerGridY + minCY, 0, height - 1);
        int maxX = std::clamp(centerGridX + maxCX, 0, width - 1);
        int maxY = std::clamp(centerGridY + maxCY, 0, height - 1);


        // ----------------------------------------------------
        // 4. 실제 셀 overlap 검사 후 blocked 처리
        // ----------------------------------------------------
        for (int y = minY; y <= maxY; ++y)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                GridCell* cell = GetCell(x, y);
                if (!cell)
                    continue;

                Vector3 cellCenter = GridToWorld(x, y);
                Vector3 cellMin = cellCenter - Vector3(cellSize * 0.5f, 0.0f, cellSize * 0.5f);
                Vector3 cellMax = cellCenter + Vector3(cellSize * 0.5f, 0.0f, cellSize * 0.5f);

                float overlapX = std::max(0.0f, std::min(cellMax.x, maxWorld.x) - std::max(cellMin.x, minWorld.x));
                float overlapZ = std::max(0.0f, std::min(cellMax.z, maxWorld.z) - std::max(cellMin.z, minWorld.z));

                float overlapArea = overlapX * overlapZ;
                float cellArea = cellSize * cellSize;

                // 셀 면적의 5% 이상 겹치면 blocked
                if (overlapArea > cellArea * 0.05f)
                {
                    cell->walkable = false;

                    int cx = x - centerGridX;
                    int cy = y - centerGridY;
                    Vector3 cellWorld = GridToWorld(x, y);

                    DebugPrintBlockedCell(
                        objName,
                        x, y,
                        cx, cy,
                        cellWorld,
                        phys->m_ColliderType);
                }
            }
        }
    }

    std::cout << "===========================================\n";
}

// [ 에디터/데이터에서 지정한 override를 다시 적용 ]
void GridComponent::BuildWalkableFromCostum()
{
    for (auto& overrideData : walkableOverrides)
    {
        SetWalkableFromCenter(overrideData.cx, overrideData.cy, overrideData.walkable);

        std::cout << "[GridComponent] Override applied: ("
            << overrideData.cx << "," << overrideData.cy << ") walkable="
            << overrideData.walkable << "\n";
    }
}

// [ Width 와 Height에 의해 재설정되는 그리드 cell ]
void GridComponent::ResizeGrid(int newWidth, int newHeight)
{
    width = newWidth;
    height = newHeight;

    cells.clear();
    cells.resize(width * height);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            GridCell& cell = cells[y * width + x];
            cell.x = x;
            cell.y = y;
            cell.walkable = true;
        }
    }
}


//----------------------------------------
// Cell 접근 / 조회
//----------------------------------------

GridCell* GridComponent::GetCell(int x, int y)
{
    if (x < 0 || y < 0 || x >= width || y >= height)
        return nullptr;

    return &cells[y * width + x];
}

bool GridComponent::IsWalkable(int x, int y)
{
    GridCell* cell = GetCell(x, y);
    return cell ? cell->walkable : false;
}


//----------------------------------------
// 중앙 기준 좌표 접근
//----------------------------------------
GridCell* GridComponent::GetCellFromCenter(int cx, int cy)
{
    int centerX = (width - 1) / 2;
    int centerY = (height - 1) / 2;

    int gridX = centerX + cx; // 중앙 기준 → 내부 배열 인덱스
    int gridY = centerY + cy;

    return GetCell(gridX, gridY);
}

bool GridComponent::IsWalkableFromCenter(int cx, int cy)
{
    GridCell* cell = GetCellFromCenter(cx, cy);
    return cell ? cell->walkable : false;
}

//----------------------------------------
// 중앙 기준 좌표로 Walkable 상태 설정
//----------------------------------------
void GridComponent::SetWalkableFromCenter(int cx, int cy, bool walkable)
{
    GridCell* cell = GetCellFromCenter(cx, cy);
    if (cell)
        cell->walkable = walkable;
}


//----------------------------------------
// Grid <-> World 변환
//----------------------------------------

// (0,0)은 좌측 하단 셀
Vector3 GridComponent::GridToWorld(int x, int y)
{
    auto transform = GetOwner()->GetTransform();
    Vector3 origin = transform->GetLocalPosition();

    float offsetX = (width * 0.5f - 0.5f) * cellSize;
    float offsetZ = (height * 0.5f - 0.5f) * cellSize;

    return {
        origin.x + (x + 0.5f) * cellSize - offsetX, // + 0.5f: 셀 중앙 기준
        origin.y,
        origin.z + (y + 0.5f) * cellSize - offsetZ
    };
}

// [ World -> Grid ]
bool GridComponent::WorldToGrid(const Vector3& pos, int& outX, int& outY)
{
    auto t = GetOwner()->GetTransform();
    Vector3 origin = t->GetLocalPosition(); 

    float offsetX = (width * 0.5f - 0.5f) * cellSize;
    float offsetZ = (height * 0.5f - 0.5f) * cellSize;

    float localX = pos.x - origin.x + offsetX;
    float localZ = pos.z - origin.z + offsetZ;

    outX = static_cast<int>(floor(localX / cellSize));
    outY = static_cast<int>(floor(localZ / cellSize));

    return !(outX < 0 || outY < 0 || outX >= width || outY >= height);
}

// [ 중앙 기준 좌표 Grid -> World ]
Vector3 GridComponent::GridToWorldFromCenter(int cx, int cy)
{
    auto transform = GetOwner()->GetTransform();
    Vector3 origin = transform->GetWorldPosition();

    return {
        origin.x + cx * cellSize,
        origin.y,
        origin.z + cy * cellSize
    };
}

bool GridComponent::WorldToGridFromCenter(const Vector3& pos, int& outCX, int& outCY)
{
    auto t = GetOwner()->GetTransform();
    Vector3 origin = t->GetWorldPosition(); // 그리드 중앙

    float localX = pos.x - origin.x;
    float localZ = pos.z - origin.z;

    outCX = static_cast<int>(floor(localX / cellSize + 0.5f));
    outCY = static_cast<int>(floor(localZ / cellSize + 0.5f));

    int centerX = (width - 1) / 2;
    int centerY = (height - 1) / 2;

    int ix = centerX + outCX;
    int iy = centerY + outCY;

    return !(ix < 0 || iy < 0 || ix >= width || iy >= height);
}


// ================================================================
// A* (에이스타) 길찾기 
// ================================================================

// 중심 좌표를 하나의 정수 key로 변환
// - allNodes / closedSet에서 동일 좌표를 빠르게 찾기 위해 사용
// - width/height 기반 offset을 사용해 음수 좌표도 다룰 수 있게 함
namespace
{
    inline int MakePathKey(int cx, int cy, int width, int height)
    {
        const int offsetX = width / 2;
        const int offsetY = height / 2;
        return (cy + offsetY) * width + (cx + offsetX);
    }

    inline float HeuristicManhattan(int fromCX, int fromCY, int toCX, int toCY)
    {
        return static_cast<float>(abs(toCX - fromCX) + abs(toCY - fromCY));
    }
}

std::vector<GridCoord> GridComponent::FindPath(int startCX, int startCY, int endCX, int endCY)
{
    std::vector<GridCoord> finalPath;

    // --------------------------------------------------------
    // 0. 사전 검사
    // --------------------------------------------------------
    // 시작점/목표점 중 하나라도 이동 불가면 경로 없음
    if (!IsWalkableFromCenter(startCX, startCY))    return finalPath;
    if (!IsWalkableFromCenter(endCX, endCY))        return finalPath;

    // --------------------------------------------------------
    // 1. A* 탐색용 자료구조
    // --------------------------------------------------------
    // openSet   : 아직 탐색 후보인 노드들 (FCost가 가장 작은 노드 우선)
    // nodeLookup : 좌표 key → 노드 포인터 (openSet 내에서 해당 좌표 노드 존재 여부 및 빠른 접근용)
    // closedSet : 이미 확정적으로 방문한 좌표
    // ownedNodes : 탐색 과정에서 생성된 노드들의 소유권 관리 (메모리 해제 용이)
    std::priority_queue<PathNode*, std::vector<PathNode*>, PathNodeCompare> openSet;
    std::unordered_map<int, PathNode*> nodeLookup;
    std::unordered_set<int> closedSet;
    std::vector<std::unique_ptr<PathNode>> ownedNodes;

    auto createNode = [&](int cx, int cy, float gCost, float hCost, PathNode* parent) -> PathNode*
        {
            ownedNodes.push_back(std::make_unique<PathNode>(PathNode{ cx, cy, gCost, hCost, parent }));
            return ownedNodes.back().get();
        };

    // 상하좌우 4방향 이동
    const std::vector<GridCoord> directions =
    {
        { 1, 0 },
        { -1, 0 },
        { 0, 1 },
        { 0, -1 }
    };

    // --------------------------------------------------------
    // 2. 시작 노드 등록
    // --------------------------------------------------------
    PathNode* startNode = createNode(
        startCX,
        startCY,
        0.0f,
        HeuristicManhattan(startCX, startCY, endCX, endCY),
        nullptr
    );

    openSet.push(startNode);
    nodeLookup[MakePathKey(startCX, startCY, width, height)] = startNode;

    // --------------------------------------------------------
    // 3. 메인 탐색 루프
    // --------------------------------------------------------
    while (!openSet.empty())
    {
        PathNode* current = openSet.top();
        openSet.pop();

        int currentKey = MakePathKey(current->cx, current->cy, width, height);

        // 이미 더 좋은 경로로 확정된 노드면 스킵
        if (closedSet.count(currentKey))
            continue;

        closedSet.insert(currentKey);

        // 목표 도착 시 경로 복원
        if (current->cx == endCX && current->cy == endCY)
        {
            while (current != nullptr)
            {
                finalPath.push_back({ current->cx, current->cy });
                current = current->parent;
            }

            std::reverse(finalPath.begin(), finalPath.end());
            return finalPath;
        }

        // ----------------------------------------------------
        // 4. 인접 노드 탐색
        // ----------------------------------------------------
        for (const auto& dir : directions)
        {
            int nextCX = current->cx + dir.cx;
            int nextCY = current->cy + dir.cy;

            // 이동 불가 셀은 스킵
            if (!IsWalkableFromCenter(nextCX, nextCY))
                continue;

            // 목표 셀이 아닌 점유 셀은 스킵
            if (IsOccupied(nextCX, nextCY) && !(nextCX == endCX && nextCY == endCY))
                continue;

            int nextKey = MakePathKey(nextCX, nextCY, width, height);

            // 이미 방문 확정된 셀은 스킵
            if (closedSet.count(nextKey))
                continue;

            float newGCost = current->gCost + 1.0f;
            float newHCost = HeuristicManhattan(nextCX, nextCY, endCX, endCY);

            auto found = nodeLookup.find(nextKey);

            // 아직 생성되지 않은 좌표면 새 노드 생성
            if (found == nodeLookup.end())
            {
                PathNode* nextNode = createNode(
                    nextCX,
                    nextCY,
                    newGCost,
                    newHCost,
                    current
                );

                nodeLookup[nextKey] = nextNode;
                openSet.push(nextNode);
                continue;
            }

            // 이미 생성된 노드인데 더 좋은 경로를 찾은 경우 갱신
            PathNode* nextNode = found->second;
            if (newGCost < nextNode->gCost)
            {
                nextNode->gCost = newGCost;
                nextNode->hCost = newHCost;
                nextNode->parent = current;

                // priority_queue는 내부 우선순위 갱신이 없으므로 다시 push
                openSet.push(nextNode);
            }
        }
    }

    // 목표 도달 실패
    return finalPath;
}


// -------------------------------------------------------------------
// [ Occupancy ]
// -------------------------------------------------------------------

int GridComponent::MakeKey(int cx, int cy)
{
    return (cy + 1000) * 2000 + (cx + 1000); // 범위 충분히 크게 
}

bool GridComponent::IsOccupied(int cx, int cy)
{
    int key = MakeKey(cx, cy);
    return occupiedCells.find(key) != occupiedCells.end();
}

void GridComponent::Occupy(int cx, int cy, AgentComponent* agent)
{
    int key = MakeKey(cx, cy);
    occupiedCells[key] = agent;
}

void GridComponent::Release(int cx, int cy)
{
    int key = MakeKey(cx, cy);
    occupiedCells.erase(key);
}

AgentComponent* GridComponent::GetOccupier(int cx, int cy)
{
    int key = MakeKey(cx, cy);
    auto it = occupiedCells.find(key);
    if (it != occupiedCells.end())
        return it->second;
    return nullptr;
}
