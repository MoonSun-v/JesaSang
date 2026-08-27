#include "AgentSystem.h"
#include "../Components/AgentComponent.h"

bool AgentSystem::ContainsAgent(const std::vector<AgentComponent*>& container, AgentComponent* agent)
{
    return std::find(container.begin(), container.end(), agent) != container.end();
}

void AgentSystem::EraseAgent(std::vector<AgentComponent*>& container, AgentComponent* agent)
{
    container.erase(std::remove(container.begin(), container.end(), agent), container.end());
}

void AgentSystem::Register(AgentComponent* a)
{
    if (!a)
        return;

    if (!isUpdating)
    {
        if (!ContainsAgent(agents, a))
            agents.push_back(a);
        return;
    }

    // 같은 FixedUpdate 안에서 다시 활성화되면 예약된 제거를 취소한다.
    EraseAgent(pendingRemoveAgents, a);

    if (!ContainsAgent(agents, a) && !ContainsAgent(pendingAddAgents, a))
        pendingAddAgents.push_back(a);
}

void AgentSystem::UnRegister(AgentComponent* a)
{
    if (!a)
        return;

    if (!isUpdating)
    {
        EraseAgent(agents, a);
        EraseAgent(pendingAddAgents, a);
        EraseAgent(pendingRemoveAgents, a);
        return;
    }

    // 아직 실제 목록에 들어가지 않은 Agent라면 등록 예약만 취소한다.
    EraseAgent(pendingAddAgents, a);

    if (ContainsAgent(agents, a) && !ContainsAgent(pendingRemoveAgents, a))
        pendingRemoveAgents.push_back(a);
}

void AgentSystem::ApplyPendingChanges()
{
    for (AgentComponent* agent : pendingRemoveAgents)
        EraseAgent(agents, agent);
    pendingRemoveAgents.clear();

    for (AgentComponent* agent : pendingAddAgents)
    {
        if (agent && !ContainsAgent(agents, agent))
            agents.push_back(agent);
    }
    pendingAddAgents.clear();
}

void AgentSystem::FixedUpdate(float dt)
{
    isUpdating = true;

    for (AgentComponent* agent : agents)
    {
        if (!agent || ContainsAgent(pendingRemoveAgents, agent))
            continue;

        agent->OnFixedUpdate(dt);
    }

    isUpdating = false;
    ApplyPendingChanges();
}
