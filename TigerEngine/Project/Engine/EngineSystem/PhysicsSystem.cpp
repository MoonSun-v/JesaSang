#include "PhysicsSystem.h"
#include "../Util/PhysXUtils.h"
#include "../Util/PhysicsLayerMatrix.h"
#include "../Components/PhysicsComponent.h"
#include <Helper.h>


// ------------------------------------------------------------
// PhysicsSystem
// ------------------------------------------------------------

PhysicsSystem::~PhysicsSystem()
{
    Shutdown();
}

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

    // sceneDesc.simulationEventCallback = &m_SimulationEventCallback;
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


    PhysicsLayerMatrix::Initialize();

    return true;
}

void PhysicsSystem::Simulate(float dt)
{
    if (!m_Scene)
        return;

    m_Scene->simulate(dt);      // 물리 연산 요청 (비동기)
    m_Scene->fetchResults(true);// 결과가 끝날 때까지 대기 후 적용

}


void PhysicsSystem::RegisterComponent(PxRigidActor* actor, PhysicsComponent* comp)
{
    if (actor) m_ActorMap[actor] = comp;
}
void PhysicsSystem::UnregisterComponent(PxActor* actor)
{
    if (!actor) return;
    m_ActorMap.erase(actor);
}
PhysicsComponent* PhysicsSystem::GetComponent(PxActor* actor)
{
    auto it = m_ActorMap.find(actor);
    return (it != m_ActorMap.end()) ? it->second : nullptr;
}


void PhysicsSystem::Shutdown()
{
    // PX_RELEASE(m_ControllerManager);
    PX_RELEASE(m_Scene);
    PX_RELEASE(m_Dispatcher);
    PX_RELEASE(m_DefaultMaterial);
    PX_RELEASE(m_Physics);

    if (m_Pvd) m_Pvd->disconnect();

    PX_RELEASE(m_PvdTransport);
    PX_RELEASE(m_Pvd);
    PX_RELEASE(m_Foundation);
}
