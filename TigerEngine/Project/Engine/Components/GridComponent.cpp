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

void DebugPrintBlock(
    const std::string& obj,
    int gx, int gy,
    int cx, int cy,
    const Vector3& worldPos,
    ColliderType type)
{
    std::cout
        << "[" << obj << "]  "
        << "Grid(" << gx << "," << gy << ")  "
        << "Center(" << cx << "," << cy << ")  "
        << "World(" << worldPos.x << "," << worldPos.z << ")  "
        << "Type(" << (int)type << ")\n";
}

void GridComponent::OnInitialize()
{
    // 셀 배열 초기화 
    ResizeGrid(width, height);

    // Physics 기반 자동 차단
    // BuildBlockedFromPhysics();
    m_pendingBuild = true;
    m_lastActorCount = -1;
     
    // 임의로 (-1,2) 그리드를 걸을 수 없게 설정
    //SetWalkableFromCenter(-1, 2, false);

    SetWalkableFromCenter(0, 3, true);
    SetWalkableFromCenter(8, -11, true);
    SetWalkableFromCenter(8, -12, true);
    SetWalkableFromCenter(9, -11, true);
    SetWalkableFromCenter(9, -12, true);
    // SetWalkableFromCenter(8, -11, true);

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

void GridComponent::BuildBlockedFromPhysics()
{
    for (auto& c : cells)
        c.walkable = true;

    auto& map = PhysicsSystem::Instance().m_ActorMap;

    std::cout << "\n==== Physics → Grid Mapping (AABB based, Center Origin) ====\n";

    int centerX = (width - 1) / 2;
    int centerY = (height - 1) / 2;

    std::unordered_set<GameObject*> visited;

    for (auto& pair : map)
    {
        PhysicsComponent* phys = pair.first;
        if (!phys || !phys->m_Actor) continue;
        if (phys->IsTrigger()) continue;
        if (phys->GetLayer() & CollisionLayer::Ground) continue;

        auto owner = phys->GetOwner();
        std::string objName = owner ? owner->GetName() : "Unknown";

        // 중복 PhysicsComponent 방어
        if (visited.count(owner)) continue;
        visited.insert(owner);

        Transform* tr = phys->transform;
        if (!tr) continue;

        Vector3 pos = tr->GetLocalPosition();

        // -----------------------
        // 1. 월드 AABB
        // -----------------------
        Vector3 minW, maxW;

        if (phys->m_ColliderType == ColliderType::Box)
        {
            PxBounds3 b = phys->m_Actor->getWorldBounds();

            std::cout << "\n[" << objName << "] PxBounds\n";
            std::cout << "  min = (" << b.minimum.x * 100 << ", " << b.minimum.y * 100 << ", " << b.minimum.z * 100 << ")\n";
            std::cout << "  max = (" << b.maximum.x * 100 << ", " << b.maximum.y * 100 << ", " << b.maximum.z * 100 << ")\n";

            std::cout << "  TransformPos = (" << pos.x << ", " << pos.y << ", " << pos.z << ")\n";

            minW = { b.minimum.x * 100, 0, b.minimum.z * 100 };
            maxW = { b.maximum.x * 100, 0, b.maximum.z * 100 };
        }
        else
        {
            float r = 0.0f;
            switch (phys->m_ColliderType)
            {
            case ColliderType::Sphere:  r = phys->m_Radius; break;
            case ColliderType::Capsule: r = phys->m_Radius; break;
            default: r = phys->m_HalfExtents.x; break;
            }

            minW = { pos.x - r, 0, pos.z - r };
            maxW = { pos.x + r, 0, pos.z + r };
        }

        // -----------------------
        // 2. AABB → 중앙 기준 Grid
        // -----------------------
        int minCX, minCY;
        int maxCX, maxCY;

        if (!WorldToGridFromCenter(minW, minCX, minCY)) continue;
        if (!WorldToGridFromCenter(maxW, maxCX, maxCY)) continue;

        std::cout << "\n[" << objName << "] "
            << "CenterGridRange: (" << minCX << "," << minCY
            << ") ~ (" << maxCX << "," << maxCY << ")\n";

        // -----------------------
        // 3. 중앙 기준 → 내부 인덱스 변환
        // -----------------------
        int minX = centerX + minCX;
        int minY = centerY + minCY;
        int maxX = centerX + maxCX;
        int maxY = centerY + maxCY;

        for (int y = minY; y <= maxY; ++y)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                if (auto* cell = GetCell(x, y))
                {
                    Vector3 cellCenter = GridToWorld(x, y);
                    Vector3 cellMin = cellCenter - Vector3(cellSize * 0.5f, 0, cellSize * 0.5f);
                    Vector3 cellMax = cellCenter + Vector3(cellSize * 0.5f, 0, cellSize * 0.5f);

                    float overlapX = std::max(0.f, std::min(cellMax.x, maxW.x) - std::max(cellMin.x, minW.x));
                    float overlapZ = std::max(0.f, std::min(cellMax.z, maxW.z) - std::max(cellMin.z, minW.z));

                    float overlapArea = overlapX * overlapZ;
                    float cellArea = cellSize * cellSize;

                    // 5% 이상 겹칠 때만 막기
                    if (overlapArea > cellArea * 0.05f)
                    {
                        cell->walkable = false;

                        int cx = x - centerX;
                        int cy = y - centerY;
                        Vector3 cellWorld = GridToWorld(x, y);

                        DebugPrintBlock(objName, x, y, cx, cy, cellWorld, phys->m_ColliderType);
                    }
                }
            }
        }
    }

    std::cout << "===========================================\n";
}

void GridComponent::BuildWalkableFromCostum()
{
    for (auto& ovr : walkableOverrides)
    {
        SetWalkableFromCenter(ovr.cx, ovr.cy, ovr.walkable);
        std::cout << "[GridComponent] Override applied: ("
            << ovr.cx << "," << ovr.cy << ") walkable="
            << ovr.walkable << "\n";
    }
}

// Width 와 Height에 의해 재설정되는 그리드 cell 
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
            auto& c = cells[y * width + x];
            c.x = x;
            c.y = y;
            c.walkable = true;
        }
    }
}


//----------------------------------------
// 셀 접근 / 조회
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

    int ix = centerX + cx; // 중앙 기준 → 내부 배열 인덱스
    int iy = centerY + cy;

    return GetCell(ix, iy);
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

// 0,0이 좌측 하단 기준
Vector3 GridComponent::GridToWorld(int x, int y)
{
    auto t = GetOwner()->GetTransform();
    // Vector3 origin = t->GetWorldPosition();
    Vector3 origin = t->GetLocalPosition();

    float offsetX = (width * 0.5f - 0.5f) * cellSize;
    float offsetZ = (height * 0.5f - 0.5f) * cellSize;

    return {
        origin.x + (x + 0.5f) * cellSize - offsetX, // +0.5f: 셀 중앙 기준
        origin.y,
        origin.z + (y + 0.5f) * cellSize - offsetZ
    };
}

// World -> Grid
bool GridComponent::WorldToGrid(const Vector3& pos, int& outX, int& outY)
{
    auto t = GetOwner()->GetTransform();
    Vector3 origin = t->GetLocalPosition(); // 그리드 중심

    float offsetX = (width * 0.5f - 0.5f) * cellSize;
    float offsetZ = (height * 0.5f - 0.5f) * cellSize;

    float localX = pos.x - origin.x + offsetX;
    float localZ = pos.z - origin.z + offsetZ;

    outX = (int)floor(localX / cellSize);
    outY = (int)floor(localZ / cellSize);

    return !(outX < 0 || outY < 0 || outX >= width || outY >= height);
}

// 중앙 기준 좌표 -> World
Vector3 GridComponent::GridToWorldFromCenter(int cx, int cy)
{
    auto t = GetOwner()->GetTransform();
    Vector3 origin = t->GetWorldPosition(); // 오브젝트 위치 = 그리드 중앙

    // cx, cy: 중앙 기준 좌표 (-center ~ +center)
    float offsetX = cx * cellSize;
    float offsetZ = cy * cellSize;

    return {
        origin.x + offsetX,
        origin.y,
        origin.z + offsetZ
    };
}

bool GridComponent::WorldToGridFromCenter(const Vector3& pos, int& outCX, int& outCY)
{
    auto t = GetOwner()->GetTransform();
    Vector3 origin = t->GetWorldPosition(); // 그리드 중앙

    float localX = pos.x - origin.x;
    float localZ = pos.z - origin.z;

    outCX = (int)floor(localX / cellSize + 0.5f);
    outCY = (int)floor(localZ / cellSize + 0.5f);

    int centerX = (width - 1) / 2;
    int centerY = (height - 1) / 2;

    int ix = centerX + outCX;
    int iy = centerY + outCY;

    return !(ix < 0 || iy < 0 || ix >= width || iy >= height);
}


// ================================================================
// A* (에이스타) 길찾기 
// ================================================================

#include <queue>
#include <unordered_map>
#include <functional>

struct Node
{
    int x, y;
    float gCost, hCost;
    Node* parent = nullptr;

    float fCost() const { return gCost + hCost; }
};

struct NodeCmp
{
    bool operator()(const Node* a, const Node* b) const
    {
        return a->fCost() > b->fCost(); // min-heap
    }
};

// 중앙 기준 좌표로 시작/목표 받아 -> walkable 한 칸씩 이동하는 경로를 반환
std::vector<std::pair<int, int>> GridComponent::FindPath(int startCX, int startCY, int endCX, int endCY)
{
    std::vector<std::pair<int, int>> finalPath;

    auto cmp = [](Node* a, Node* b) { return a->fCost() > b->fCost(); };
    std::priority_queue<Node*, std::vector<Node*>, NodeCmp> openSet;
    std::unordered_map<int, Node*> allNodes;

    auto hash = [this](int x, int y) { return (y + (width / 2)) * width + (x + (width / 2)); };

    Node* start = new Node{ startCX, startCY, 0, float(abs(endCX - startCX) + abs(endCY - startCY)), nullptr };
    openSet.push(start);
    allNodes[hash(startCX, startCY)] = start;

    std::vector<std::pair<int, int>> directions = { {1,0},{-1,0},{0,1},{0,-1} };

    while (!openSet.empty())
    {
        Node* current = openSet.top();
        openSet.pop();

        if (current->x == endCX && current->y == endCY)
        {
            // 경로 역추적
            while (current)
            {
                finalPath.push_back({ current->x, current->y });
                current = current->parent;
            }
            std::reverse(finalPath.begin(), finalPath.end());
            break;
        }

        for (auto& dir : directions)
        {
            int nx = current->x + dir.first;
            int ny = current->y + dir.second;

            if (!IsWalkableFromCenter(nx, ny)) continue;

            int key = hash(nx, ny);
            float gNew = current->gCost + 1;

            if (allNodes.find(key) == allNodes.end() || gNew < allNodes[key]->gCost)
            {
                float h = float(abs(endCX - nx) + abs(endCY - ny));
                Node* neighbor = new Node{ nx, ny, gNew, h, current };
                openSet.push(neighbor);
                allNodes[key] = neighbor;
            }
        }
    }

    // 동적할당 해제
    for (auto& pair : allNodes) delete pair.second;

    return finalPath;
}
