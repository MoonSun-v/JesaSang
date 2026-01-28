#include "PhysicsSystem.h"
#include "CharacterControllerSystem.h"
#include "../Util/PhysXUtils.h"
#include "../Util/PhysicsLayerMatrix.h"
#include "../Components/PhysicsComponent.h"
#include <Helper.h>


// ------------------------------------------------------------
// PhysicsSystem
// ------------------------------------------------------------

bool PhysicsSystem::Initialize()
{
    // ------------------------------------------------------
    // 1. Foundation : PhysX의 모든 객체는 Foundation 위에서 동작
    // ------------------------------------------------------
    m_Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_Allocator, m_ErrorCallback);
    if (!m_Foundation)
    {
        LOG_ERRORA("PxCreateFoundation failed!");
        return false;
    }

    // ------------------------------------------------------
    // 2. PVD
    // ------------------------------------------------------
    m_Pvd = PxCreatePvd(*m_Foundation);
    m_PvdTransport = PxDefaultPvdSocketTransportCreate( // 로컬 PC의 PVD 프로그램과 소켓으로 연결 
        "127.0.0.1", // IP
        5425,        // 포트 (PVD 기본값)
        10           // 연결 타임아웃(ms)
    );
    m_Pvd->connect(*m_PvdTransport, PxPvdInstrumentationFlag::eALL); // 모든 디버깅 데이터 전송 

    // ------------------------------------------------------
    // 3. Physics 객체 생성 
    // ------------------------------------------------------
    m_Physics = PxCreatePhysics(
        PX_PHYSICS_VERSION,
        *m_Foundation,
        PxTolerancesScale(), // 길이/질량 기준 스케일
        true,                // PhysX Extensions 사용
        m_Pvd                // PVD 연결
    );
    if (!m_Physics)
    {
        LOG_ERRORA("PxCreatePhysics failed!");
        return false;
    }

    // ------------------------------------------------------
    // 4. Scene 생성 
    // ------------------------------------------------------
    PxSceneDesc sceneDesc(m_Physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.f, -9.81f, 0.f); // 중력 설정 (Y축 아래 방향)

    sceneDesc.simulationEventCallback = &m_SimulationEventCallback;
    m_Dispatcher = PxDefaultCpuDispatcherCreate(2); // CPU 물리 연산을 담당할 스레드 풀 (2 스레드)
    sceneDesc.cpuDispatcher = m_Dispatcher;
    sceneDesc.filterShader = PhysicsFilterShader;

    m_Scene = m_Physics->createScene(sceneDesc);
    if (!m_Scene)
        return false;

    // ------------------------------------------------------
    // 5. PVD Scene 설정 (있어야 충돌/접촉 보임)
    // ------------------------------------------------------
    if (PxPvdSceneClient* client = m_Scene->getScenePvdClient())
    {
        client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }

    // ------------------------------------------------------
    // 6. Default Material
    // ------------------------------------------------------
    m_DefaultMaterial = m_Physics->createMaterial(
        0.5f, // 정지 마찰력 (static friction)
        0.5f, // 동적 마찰력 (dynamic friction)
        0.6f  // 반발력 (restitution)
    );


    // ------------------------------------------------------
    // 7. Character Controller Manager
    // ------------------------------------------------------
    CharacterControllerSystem::Instance().Initialize(m_Scene);


    // ------------------------------------------------------
    // 8. 피직스 레이어 
    // ------------------------------------------------------
    PhysicsLayerMatrix::Initialize();

    return true;
}


void PhysicsSystem::Simulate(float dt)
{
    if (!m_Scene)
        return;

    // 1. PhysX 시뮬레이션
    m_Scene->simulate(dt);      // 물리 연산 요청 (비동기)
    m_Scene->fetchResults(true);// 결과가 끝날 때까지 대기 후 적용

    // 2. Actor 위치 동기화
    for (auto& it : m_ActorMap)
    {
        PhysicsComponent* comp = it.first;
        if (comp)
            comp->SyncFromPhysics();
    }

    // 3. Trigger 체크
    for (auto& it : m_ActorMap)
    {
        PhysicsComponent* comp = it.first;
        if (comp)
            comp->CheckTriggers();
    }

    // 4. CCT 후처리 (Trigger / Collision 이벤트)
    CharacterControllerSystem::Instance().Simulate(dt);

    // 4. Trigger 이벤트 해석
    ResolveTriggerEvents();
}


void PhysicsSystem::ResolveTriggerEvents()
{
    m_TriggerCurr.clear();

    // --------------------------------------------------
    // 1. 모든 PhysicsComponent에서 Trigger 수집
    // --------------------------------------------------
    for (auto& it : m_ActorMap)
    {
        PhysicsComponent* comp = it.first;

        if (!comp)
            continue;

        for (PhysicsComponent* other : comp->m_PendingTriggers)
        {
            // (A,B) == (B,A) 정규화
            PhysicsComponent* a = comp < other ? comp : other;
            PhysicsComponent* b = comp < other ? other : comp;
            m_TriggerCurr.insert(std::make_pair(a, b));
        }

        comp->m_PendingTriggers.clear();
    }

    // --------------------------------------------------
    // 2. Trigger Enter / Stay
    // --------------------------------------------------
    for (const auto& pair : m_TriggerCurr)
    {
        if (m_TriggerPrev.find(pair) == m_TriggerPrev.end())
        {
            pair.first->OnTriggerEnter(pair.second);
            pair.second->OnTriggerEnter(pair.first);
        }
        else
        {
            pair.first->OnTriggerStay(pair.second);
            pair.second->OnTriggerStay(pair.first);
        }
    }

    // --------------------------------------------------
    // 3. Trigger Exit
    // --------------------------------------------------
    for (const auto& pair : m_TriggerPrev)
    {
        if (m_TriggerCurr.find(pair) == m_TriggerCurr.end())
        {
            pair.first->OnTriggerExit(pair.second);
            pair.second->OnTriggerExit(pair.first);
        }
    }
    m_TriggerPrev = std::move(m_TriggerCurr);
}


void PhysicsSystem::RegisterComponent(PhysicsComponent* comp, PxRigidActor* actor)
{
    m_ActorMap[comp] = actor;
}

void PhysicsSystem::UnregisterComponent(PhysicsComponent* comp)
{
    if (!comp) return;

    auto it = m_ActorMap.find(comp);
    if (it == m_ActorMap.end())
        return;

    PxActor* actor = it->second;
    m_ActorMap.erase(it);

    if (actor)
        actor->release();

    comp->m_Actor = nullptr;
}

PhysicsComponent* PhysicsSystem::GetComponent(PxActor* actor)
{
    for (auto& it : m_ActorMap)
    {
        if (it.second == actor) return it.first;
    }
    return nullptr;

}

void PhysicsSystem::Shutdown()
{
    for (auto& it : m_ActorMap)
    {
        if (it.second)
            it.second->release();
    }
    m_ActorMap.clear();

    CharacterControllerSystem::Instance().Shutdown();
    PX_RELEASE(m_Scene);
    PX_RELEASE(m_Dispatcher);
    PX_RELEASE(m_DefaultMaterial);
    PX_RELEASE(m_Physics);

    if (m_Pvd) m_Pvd->disconnect();
    PX_RELEASE(m_PvdTransport);
    PX_RELEASE(m_Pvd);
    PX_RELEASE(m_Foundation);
}

// ------------------------------------------------------------
// Raycast Query 필터
// - Trigger 포함 여부 제어
// - Raycast 레이어 <-> Actor 레이어 양방향 검사
// ------------------------------------------------------------
PxQueryHitType::Enum RaycastFilterCallback::preFilter(
    const PxFilterData& filterData,
    const PxShape* shape,
    const PxRigidActor* actor,
    PxHitFlags&)
{
    if (!shape || !actor)
        return PxQueryHitType::eNONE;

    // Trigger 처리 
    bool isTrigger = shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE;
    if (isTrigger && m_TriggerInteraction == QueryTriggerInteraction::Ignore)
        return PxQueryHitType::eNONE;

    // Actor의 PhysicsComponent 매핑 
    PhysicsComponent* comp = PhysicsSystem::Instance().GetComponent(const_cast<PxRigidActor*>(actor));
    if (!comp)
    {
        return PxQueryHitType::eNONE;
    }

    // Raycast 레이어 필터링 : CanCollide 대신 안전하게 양방향 체크
    CollisionMask rayMask = PhysicsLayerMatrix::GetMask(m_RaycastLayer);    // rayMask:   Raycast가 감지할 수 있는 레이어들의 마스크
    CollisionMask actorMask = PhysicsLayerMatrix::GetMask(comp->GetLayer());// actorMask: Actor가 충돌할 수 있는 레이어들의 비트 마스크

    if (((rayMask & (uint32_t)comp->GetLayer()) == 0) || ((actorMask & (uint32_t)m_RaycastLayer) == 0))
    {
        return PxQueryHitType::eNONE;
    }

    // 단일 감지 vs 다중 감지 
    if (m_AllHits)
        return PxQueryHitType::eTOUCH; // Ray 계속 진행
    else
        return PxQueryHitType::eBLOCK; // 여기서 Ray 종료
}

// Narrow Phase 이후 추가 필터 없음 -> 전부 허용 
PxQueryHitType::Enum RaycastFilterCallback::postFilter(
    const PxFilterData&,
    const PxQueryHit&,
    const PxShape*,
    const PxRigidActor*)
{
    return PxQueryHitType::eBLOCK;
}


// ----------------------------------------------------
// [ Simulation Event Callback ] 
// ----------------------------------------------------

void SimulationEventCallback::onContact(
    const PxContactPairHeader& pairHeader,
    const PxContactPair* pairs,
    PxU32 nbPairs)
{
    auto* compA = PhysicsSystem::Instance().GetComponent(pairHeader.actors[0]);
    auto* compB = PhysicsSystem::Instance().GetComponent(pairHeader.actors[1]);

    if (!compA || !compB)
        return;

    if (!PhysicsLayerMatrix::CanCollide(
        compA->GetLayer(),
        compB->GetLayer()))
    {
        return;
    }

    for (PxU32 i = 0; i < nbPairs; i++)
    {
        const PxContactPair& pair = pairs[i];

        if (pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            compA->OnCollisionEnter(compB);
            compB->OnCollisionEnter(compA);
        }

        if (pair.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
        {
            compA->OnCollisionStay(compB);
            compB->OnCollisionStay(compA);
        }

        if (pair.events & PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            compA->OnCollisionExit(compB);
            compB->OnCollisionExit(compA);
        }
    }
}


// ----------------------------------------------------
// [ Raycast ] 
// ----------------------------------------------------

bool PhysicsSystem::Raycast(
    const PxVec3& origin,
    const PxVec3& direction,
    float maxDistance,
    std::vector<RaycastHit>& outHits,
    CollisionLayer layer,
    QueryTriggerInteraction triggerInteraction,
    bool bAllHits)
{
    outHits.clear();
    if (!m_Scene) return false;

    // 최대 히트 수 지정
    const PxU32 maxHits = 128;
    PxRaycastHit hitArray[maxHits];
    PxRaycastBuffer hitBuffer(hitArray, maxHits);

    // Broad Phase 필터
    PxQueryFilterData filterData;
    filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;
    filterData.data.word0 = static_cast<PxU32>(layer);            // Raycast 레이어
    filterData.data.word1 = PhysicsLayerMatrix::GetMask(layer);   // Actor가 충돌 가능한 레이어 마스크

    RaycastFilterCallback filterCallback(layer, triggerInteraction, bAllHits);

    // MeshMultiple 플래그로 모든 히트를 한 번에 수집
    PxHitFlags hitFlags = PxHitFlag::eDEFAULT | PxHitFlag::eMESH_MULTIPLE;

    bool bHit = m_Scene->raycast(
        origin,
        direction.getNormalized(),
        maxDistance,
        hitBuffer,
        hitFlags,
        filterData,
        &filterCallback
    );

    if (!bHit) return false;

    // Actor 중복 제거용 Set (유니티에서는 Actor 단위로 한 번만 반환한다고 함)
    std::unordered_set<PxRigidActor*> hitActors;

    auto ProcessHit = [&](const PxRaycastHit& pxHit)
        {
            PxShape* shape = pxHit.shape;
            PxRigidActor* actor = pxHit.actor;
            if (!actor || !shape) return;

            if (hitActors.find(actor) != hitActors.end()) return; // 이미 처리됨
            hitActors.insert(actor);

            PhysicsComponent* comp = GetComponent(pxHit.actor);
            if (!comp) return;

            bool isTrigger = shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE;
            if (isTrigger && triggerInteraction == QueryTriggerInteraction::Ignore)
                return;

            if (!PhysicsLayerMatrix::CanCollide(layer, comp->GetLayer()))
                return;

            RaycastHit hitInfo;
            hitInfo.component = comp;
            hitInfo.point = pxHit.position;
            hitInfo.normal = pxHit.normal;
            hitInfo.distance = pxHit.distance;
            hitInfo.shape = shape;
            hitInfo.actor = pxHit.actor;

            outHits.push_back(hitInfo);
        };

    if (bAllHits)
    {
        // RaycastAll : TOUCH 결과만 처리
        for (PxU32 i = 0; i < hitBuffer.getNbAnyHits(); ++i)
        {
            ProcessHit(hitBuffer.getAnyHit(i));
        }
    }
    else
    {
        // Raycast : BLOCK 하나만 처리
        if (hitBuffer.hasBlock)
        {
            ProcessHit(hitBuffer.block);
        }
    }

    return !outHits.empty();
}

void PhysicsSystem::DrawPhysXActors()
{
    for (auto& it : m_ActorMap)
    {
        PhysicsComponent* comp = it.first;
        if (comp)
            comp->DrawPhysXActors();
    }
}
