#pragma once
#include <PxPhysicsAPI.h>
#include <task/PxCpuDispatcher.h>
#include <unordered_map>
#include <unordered_set>

#include "../Util/PhysicsFilterShader.h"
#include "../Util/RaycastHit.h"
#include "../Util/CollisionLayer.h"

#include "System/Singleton.h"

using namespace physx;

class PhysicsComponent;
class PhysicsLayerMatrix;
struct PairHash
{
    size_t operator()(const std::pair<PhysicsComponent*, PhysicsComponent*>& p) const
    {
        return std::hash<void*>()(p.first) ^ std::hash<void*>()(p.second);
    }
};

enum class QueryTriggerInteraction
{
    UseGlobal, // (지금은 Collide와 동일 취급)
    Ignore,    // Trigger 무시
    Collide    // Trigger 포함
};

// ----------------------------------------------------
// [ SimulationEventCallback ] 
// 
// 각 Shape에 설정된 isTrigger에 따라서 이벤트 실행
//  - PxSimulationEventCallback 을 상속받아 구현한다. 
// ----------------------------------------------------
class SimulationEventCallback : public PxSimulationEventCallback
{
public:
    // Simulation Shape ↔ Simulation Shape
    virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;

    // 사용 안 함
        // Trigger 이벤트는 PhysX Simulation Trigger를 사용X 
        // 모든 Trigger는 Overlap Query + PendingTriggers 방식으로 통합 처리O 
    virtual void onTrigger(PxTriggerPair* pairs, PxU32 nbPairs) override {} // Trigger Shape ↔ Simulation Shape
    virtual void onConstraintBreak(PxConstraintInfo*, PxU32) override {}
    virtual void onWake(PxActor**, PxU32) override {}
    virtual void onSleep(PxActor**, PxU32) override {}
    virtual void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) override {}
};


// ------------------------------
// Raycast Filter
// ------------------------------
class RaycastFilterCallback : public PxQueryFilterCallback
{
public:
    RaycastFilterCallback(
        CollisionLayer raycastLayer,
        QueryTriggerInteraction triggerInteraction,
        bool bAllHits)
        : m_RaycastLayer(raycastLayer)
        , m_TriggerInteraction(triggerInteraction)
        , m_AllHits(bAllHits)
    {
    }

    PxQueryHitType::Enum preFilter(
        const PxFilterData& filterData,
        const PxShape* shape,
        const PxRigidActor* actor,
        PxHitFlags& queryFlags) override;

    PxQueryHitType::Enum postFilter(
        const PxFilterData& filterData,
        const PxQueryHit& hit,
        const PxShape* shape,
        const PxRigidActor* actor) override;

private:
    CollisionLayer m_RaycastLayer;
    QueryTriggerInteraction m_TriggerInteraction;
    bool m_AllHits;
};


// ----------------------------------------------------
// [ PhysicsSystem ] 
// 
// PhysX 기반 물리 시스템 관리
//  - PhysX Foundation / Physics / Scene 생성 및 관리
//  - 물리 시뮬레이션 업데이트 
//  - RigidBody + Trigger + Raycast 
// ----------------------------------------------------
class PhysicsSystem : public Singleton<PhysicsSystem>
{
public:
    PhysicsSystem(token) {}
    ~PhysicsSystem() = default;

    bool Initialize();
    void Simulate(float dt); // 물리 시뮬레이션 1프레임 수행 
    void Shutdown();

    void RegisterComponent(PxRigidActor* actor, PhysicsComponent* comp);
    void UnregisterComponent(PxActor* actor);

    // getter 
    PxPhysics* GetPhysics() const { return m_Physics; }
    PxScene* GetScene()   const { return m_Scene; }
    PxMaterial* GetDefaultMaterial() const { return m_DefaultMaterial; }
    PhysicsComponent* GetComponent(PxActor* actor);

    // Actor <-> Component 매핑
    std::unordered_map<PxActor*, PhysicsComponent*> m_ActorMap;

    std::unordered_set<std::pair<PhysicsComponent*, PhysicsComponent*>, PairHash> m_TriggerCurr;
    std::unordered_set<std::pair<PhysicsComponent*, PhysicsComponent*>, PairHash> m_TriggerPrev;

    void ResolveTriggerEvents();


private:
    // ------------------------------------------------------
    // PhysX 기본 유틸 객체 
    // ------------------------------------------------------
    // PxDefaultAllocator       : PhysX 내부 메모리 할당자
    // PxDefaultErrorCallback   : PhysX 에러/경고 콜백 (콘솔 출력용)
    PxDefaultAllocator      m_Allocator;
    PxDefaultErrorCallback  m_ErrorCallback;

    // ------------------------------------------------------
    // PhysX 핵심 객체 
    // ------------------------------------------------------
    // PxFoundation         : PhysX의 최상위 객체 (가장 먼저 생성, 가장 나중에 파괴)
    // PxPhysics            : 물리 연산의 핵심 객체 (RigidBody, Shape 생성 담당)
    // PxScene              : 실제 물리 시뮬레이션이 수행되는 공간
    // PxMaterial           : 기본 물리 재질 (마찰, 반발력)
    // PxDefaultCpuDispatcher : CPU 멀티스레드 디스패처
    PxFoundation* m_Foundation = nullptr;
    PxPhysics* m_Physics = nullptr;
    PxScene* m_Scene = nullptr;
    PxMaterial* m_DefaultMaterial = nullptr;
    PxDefaultCpuDispatcher* m_Dispatcher = nullptr;

    // ------------------------------------------------------
    // PVD (PhysX Visual Debugger) 관련
    // ------------------------------------------------------
    PxPvd* m_Pvd = nullptr;
    PxPvdTransport* m_PvdTransport = nullptr;

    // -----------------------------------------------------
    SimulationEventCallback m_SimulationEventCallback;


public:
    // Raycast
    bool Raycast(
        const PxVec3& origin,
        const PxVec3& direction,
        float maxDistance,
        std::vector<RaycastHit>& outHits,
        CollisionLayer layer,
        QueryTriggerInteraction triggerInteraction,
        bool bAllHits);

};