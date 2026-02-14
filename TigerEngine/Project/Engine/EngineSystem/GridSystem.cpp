#include "GridSystem.h"
#include "../Components/GridComponent.h"
#include "../EngineSystem/PhysicsSystem.h"

void GridSystem::Register(GridComponent* g)
{
    grids.push_back(g);
}

void GridSystem::UnRegister(GridComponent* g)
{
    grids.erase(std::remove(grids.begin(), grids.end(), g), grids.end());
}

void GridSystem::FixedUpdate(float delta)
{
    GridComponent* grid = GetMainGrid();
    if (!grid) return;

    if (!grid->m_pendingBuild)
        return;

    auto& map = PhysicsSystem::Instance().m_ActorMap;
    int actorCount = (int)map.size();

    if (actorCount == 0)
        return;

    if (actorCount == grid->m_lastActorCount)
    {
        // Physics 안정화 → Grid 생성
        grid->BuildBlockedFromPhysics();


        grid->BuildWalkableFromCostum();
        grid->m_pendingBuild = false;

        std::cout << "[GridSystem] Grid build completed. ActorCount = "
            << actorCount << "\n";
    }
    else
    {
        grid->m_lastActorCount = actorCount;
    }
}