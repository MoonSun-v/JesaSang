#include "CharacterControllerSystem.h"
#include "PhysicsSystem.h"
#include "../Components/CharacterControllerComponent.h"
#include "../Components/PhysicsComponent.h"
#include "../Util/PhysicsLayerMatrix.h"


// ------------------------------------------------------------
// CCT Sweep 이동 중 Shape과 충돌했을 때 호출되는 콜백
// - CCT는 RigidBody 충돌을 직접 해결하지 않고 Sweep(Query) 결과를 기반으로 처리한다.
// ------------------------------------------------------------
void ControllerHitReport::onShapeHit(const PxControllerShapeHit& hit)
{
    CharacterControllerComponent* cctComp = owner;
    PhysicsComponent* otherComp = PhysicsSystem::Instance().GetComponent(hit.actor);

    if (!cctComp || !otherComp)
        return;

    // -------------------------------------------------
    // 1. 레이어 필터 (완전 차단)
    // -------------------------------------------------
    if (!PhysicsLayerMatrix::CanCollide(cctComp->GetLayer(), otherComp->GetLayer()))
    {
        return;
    }

    // -------------------------------------------------
    // 2. Trigger는 CCT Collision 경로에서 완전 제외
    //  - Trigger는 Overlap Query(CheckCCTTriggers)에서만 처리한다.
    //  - 여기서 Trigger 허용하면 Collision + Trigger가 동시에 발생..
    // -------------------------------------------------
    if (otherComp->IsTrigger())
        return;

    // -------------------------------------------------
    // 3. Dynamic Actor 밀어내기
    // -------------------------------------------------
    PxRigidDynamic* actor = hit.actor->is<PxRigidDynamic>();
    if (actor)
    {
        // Controller → Actor 방향
        PxExtendedVec3 cctPos = hit.controller->getPosition();

        PxVec3 controllerPos(
            static_cast<PxReal>(cctPos.x),
            static_cast<PxReal>(cctPos.y),
            static_cast<PxReal>(cctPos.z));

        PxVec3 worldPos(
            static_cast<PxReal>(hit.worldPos.x),
            static_cast<PxReal>(hit.worldPos.y),
            static_cast<PxReal>(hit.worldPos.z));

        PxVec3 dir = worldPos - controllerPos;
        dir.y = 0.0f;

        if (dir.normalize() > 0.001f)
        {
            const PxF32 pushStrength = 5.0f;
            actor->addForce(dir * pushStrength, PxForceMode::eFORCE);
        }
    }

    // -------------------------------------------------
    // 4. Collision 이벤트 수집
    // -------------------------------------------------
    cctComp->m_CCTCurrContacts.insert(otherComp);
}



// ------------------------------------------------------------
// CCT Trigger Overlap Query용 필터
// - Trigger Shape만 감지
// - Layer / Mask 양방향 검사
// ------------------------------------------------------------
PxQueryHitType::Enum TriggerFilter::preFilter(
    const PxFilterData& filterData,
    const PxShape* shape,
    const PxRigidActor* actor,
    PxHitFlags&)
{
    //  Trigger Shape만 허용
    if (!(shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE))
        return PxQueryHitType::eNONE;

    const PxFilterData& shapeData = shape->getQueryFilterData();

    // 양방향 레이어 + 마스크 검사
    if (!(filterData.word1 & shapeData.word0) || !(shapeData.word1 & filterData.word0))
    {
        return PxQueryHitType::eNONE;
    }

    // 자기 자신 제외 (CCT Actor)
    if (actor == owner->m_Controller->getActor())
        return PxQueryHitType::eNONE;

    return PxQueryHitType::eTOUCH;
}

PxQueryHitType::Enum TriggerFilter::postFilter(const PxFilterData&, const PxQueryHit&, const PxShape*, const PxRigidActor*)
{
    return PxQueryHitType::eTOUCH;
}



// ------------------------------------------------------------
// CCT 이동(Sweep) 시 충돌 대상 결정
// - Trigger 완전 제외
// - Block만 충돌 대상으로 처리
// ------------------------------------------------------------
PxQueryHitType::Enum CCTQueryFilter::preFilter(
    const PxFilterData& filterData,
    const PxShape* shape,
    const PxRigidActor* actor,
    PxHitFlags&)
{
    const PxFilterData& shapeData = shape->getQueryFilterData();

    // 양방향 레이어 검사
    if (!(filterData.word1 & shapeData.word0) || !(shapeData.word1 & filterData.word0))
    {
        return PxQueryHitType::eNONE;
    }

    // Trigger는 Sweep 충돌 대상에서 제외
    if (shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
    {
        return PxQueryHitType::eNONE;
    }

    return PxQueryHitType::eBLOCK;
}

PxQueryHitType::Enum CCTQueryFilter::postFilter(
    const PxFilterData&,
    const PxQueryHit&,
    const PxShape*,
    const PxRigidActor*)
{
    return PxQueryHitType::eNONE;
}


// -------------------------------------------------------------------------

void CharacterControllerSystem::Initialize(PxScene* scene)
{
    if (!scene) return;
    m_ControllerManager = PxCreateControllerManager(*scene);
}

void CharacterControllerSystem::Shutdown()
{
    for (auto& it : m_CCTMap)
    {
        PxController* cct = it.second;
        if (cct)
            cct->release();
    }
    m_CCTMap.clear();

    PX_RELEASE(m_ControllerManager);
}

void CharacterControllerSystem::Simulate(float dt)
{
    if (!m_ControllerManager) return;

    for (auto& it : m_CCTMap)
    {
        CharacterControllerComponent* comp = it.first;
        if (comp)
        {
            comp->SyncFromController();
            comp->ResolveCollisions();
            comp->CheckTriggers();      // CCT 위치 기반 Overlap Query
            comp->ResolveTriggers();    // 수집만 진행 
        }
    }
}


void CharacterControllerSystem::RegisterComponent(CharacterControllerComponent* comp, PxController* cct)
{
    if (comp && cct)
        m_CCTMap[comp] = cct;
}

void CharacterControllerSystem::UnRegisterComponent(CharacterControllerComponent* comp)
{
    if (!comp) return;

    auto it = m_CCTMap.find(comp);
    if (it == m_CCTMap.end())
        return;

    PxController* cct = it->second;
    m_CCTMap.erase(it);

    if (cct)
        cct->release();

    comp->m_Controller = nullptr;
}


// 플레이어 전용 콜라이더 생성 
PxController* CharacterControllerSystem::CreateCapsuleCollider(
    const PxExtendedVec3& position,
    float radius,
    float height,
    float density)
{
    if (!m_ControllerManager || !PhysicsSystem::Instance().GetDefaultMaterial())
        return nullptr;

    PxCapsuleControllerDesc desc;
    desc.position = position;
    desc.radius = radius;
    desc.height = height;
    desc.material = PhysicsSystem::Instance().GetDefaultMaterial();
    desc.density = density;
    desc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
    desc.slopeLimit = cosf(PxPi / 4);   // 오를 수 있는 최대 경사 : 45도
    desc.stepOffset = 0.2f;             // 자동으로 넘을 수 있는 턱 (20cm)
    desc.contactOffset = 0.05f;         // 충돌 여유 (떨림 방지) : 값이 클수록 더 쉽게 계단으로 인식 
    desc.reportCallback = &m_ControllerHitReport;

    PxController* controller = m_ControllerManager->createController(desc);
    if (!controller)
        return nullptr;

    // Controller Actor 충돌 설정
    if (PxRigidDynamic* actor = controller->getActor())
    {
        PxShape* shapes[8];
        PxU32 count = actor->getShapes(shapes, 8);
        for (PxU32 i = 0; i < count; ++i)
        {
            // CCT Shape은 Simulation에서 제거함. 
            // - RigidBody와 직접 충돌 계산하지 않음
            // - 모든 충돌은 Sweep(Query) 기반
            shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
            shapes[i]->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
        }
    }

    return controller;
}
