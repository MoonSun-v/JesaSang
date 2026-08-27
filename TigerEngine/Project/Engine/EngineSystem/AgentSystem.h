#pragma once
#include "System/Singleton.h"
#include <vector>

class AgentComponent;

class AgentSystem : public Singleton<AgentSystem>
{
    std::vector<AgentComponent*> agents;
    std::vector<AgentComponent*> pendingAddAgents;
    std::vector<AgentComponent*> pendingRemoveAgents;
    bool isUpdating = false;

    // Agent 목록에 대상이 들어 있는지 확인
    static bool ContainsAgent(const std::vector<AgentComponent*>& container, AgentComponent* agent);

    // Agent 목록에서 대상을 제거
    static void EraseAgent(std::vector<AgentComponent*>& container, AgentComponent* agent);

    // FixedUpdate 중 예약된 등록 및 해제 요청 반영
    void ApplyPendingChanges();

public:
    std::vector<AgentComponent*>& GetAgents() { return agents; }

public:
    AgentSystem(token) {}

    void Register(AgentComponent* a);
    void UnRegister(AgentComponent* a);

    void FixedUpdate(float dt);
};
