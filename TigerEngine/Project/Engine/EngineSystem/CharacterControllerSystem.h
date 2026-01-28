#pragma once
#include <PxPhysicsAPI.h>
#include <unordered_set>
#include <unordered_map>

#include "System/Singleton.h"

using namespace physx;

class PhysicsComponent;
class CharacterControllerComponent;

// ----------------------------------------------------
// [ ControllerHitReport ] 
// 엔진 공용 PhysX 콜백
//  - onShapeHit : CCT ↔ PhysX Shape (RigidStatic / RigidDynamic) 충돌 시 호출
// ----------------------------------------------------
class ControllerHitReport : public PxUserControllerHitReport
{
public:
    CharacterControllerComponent* owner = nullptr;

    virtual void onShapeHit(const PxControllerShapeHit& hit) override;
    virtual void onControllerHit(const PxControllersHit&) override {}
    virtual void onObstacleHit(const PxControllerObstacleHit&) override {}
};

// ------------------------------
// CCT Trigger Filter (Overlap Query용)
// ------------------------------
class TriggerFilter : public PxQueryFilterCallback
{
public:
    CharacterControllerComponent* owner;
    TriggerFilter(CharacterControllerComponent* c) : owner(c) {}

    // Trigger Overlap Query 전용 필터
    //  - Trigger Shape만 통과
    //  - Layer/Mask 양방향 검사
    //  - 자기 자신(CCT Actor)은 제외
    PxQueryHitType::Enum preFilter(
        const PxFilterData& filterData,
        const PxShape* shape,
        const PxRigidActor* actor,
        PxHitFlags&) override;

    PxQueryHitType::Enum postFilter(
        const PxFilterData&, const PxQueryHit&, const PxShape*, const PxRigidActor*) override;
};

// ------------------------------
// CCT Collision Filter (Sweep용)
// ------------------------------
class CCTQueryFilter : public PxQueryFilterCallback
{
public:
    CharacterControllerComponent* owner;
    CCTQueryFilter(CharacterControllerComponent* comp) : owner(comp) {}

    PxQueryHitType::Enum preFilter(
        const PxFilterData& filterData,   // CCT FilterData
        const PxShape* shape,
        const PxRigidActor* actor,
        PxHitFlags&) override;

    PxQueryHitType::Enum postFilter(
        const PxFilterData&, const PxQueryHit&, const PxShape*, const PxRigidActor*) override;
};


// ----------------------------------------------------
// [ CharacterControllerSystem ] 
// 
// - CCT 전용 물리 시스템 관리
// ----------------------------------------------------
class CharacterControllerSystem : public Singleton<CharacterControllerSystem>
{
public:
    CharacterControllerSystem(token) {}
    ~CharacterControllerSystem() = default;

    void Initialize(PxScene* scene);
    void Simulate(float dt);
    void Shutdown();

    void RegisterComponent(CharacterControllerComponent* comp, PxController* cct);
    void UnRegisterComponent(CharacterControllerComponent* comp);

    PxControllerManager* GetControllerManager() const { return m_ControllerManager; }

    // CCT <-> Component 매핑
    std::unordered_map<CharacterControllerComponent*, PxController*> m_CCTMap;

    // getter 
    ControllerHitReport& GetHitReport() { return m_ControllerHitReport; }

private:
    PxControllerManager* m_ControllerManager = nullptr;
    ControllerHitReport m_ControllerHitReport; // CCT 전용 HitReport

    std::unordered_set<PhysicsComponent*> m_CCTCurrTriggers;


public:
    PxController* CreateCapsuleCollider(
        const PxExtendedVec3& position,
        float radius,
        float height,
        float density = 10.0f
    );
};