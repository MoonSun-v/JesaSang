#include "CharacterControllerComponent.h"
#include "../EngineSystem/CharacterControllerSystem.h"
#include "../EngineSystem/PhysicsSystem.h"
#include "../Util/PhysicsLayerMatrix.h"
#include "../Util/PhysXUtils.h"
#include "../Object/GameObject.h"

#include "Transform.h"
#include "PhysicsComponent.h"


void CharacterControllerComponent::OnInitialize()
{
    transform = GetOwner()->GetTransform();
}

CharacterControllerComponent::~CharacterControllerComponent()
{
    if (m_Controller)
    {
        CharacterControllerSystem::Instance().UnregisterComponent(m_Controller);
        PX_RELEASE(m_Controller);
    }
}

void CharacterControllerComponent::CreateCharacterCollider(float radius, float height, const Vector3& offset)
{
    if (!transform) return;

    m_Offset = offset;
    auto& phys = CharacterControllerSystem::Instance();

    PxExtendedVec3 pos(
        (transform->GetPosition().x + offset.x) * WORLD_TO_PHYSX,
        (transform->GetPosition().y + offset.y) * WORLD_TO_PHYSX,
        (transform->GetPosition().z + offset.z) * WORLD_TO_PHYSX
    );

    m_Controller = phys.CreateCapsuleCollider(
        pos,
        radius * WORLD_TO_PHYSX,
        height * WORLD_TO_PHYSX,
        10.0f   // density (사실상 무의미) density는 반드시 > 0
    );

    phys.RegisterComponent(m_Controller, this);

    SetLayer(CollisionLayer::Default); // 초기 레이어 적용
}

void CharacterControllerComponent::MoveCharacter(const Vector3& wishDir, float fixedDt)
{
    if (!m_Controller)
    {
        OutputDebugStringW(L"[CharacterControllerComponent] MoveCharacter의 m_Controller가 null입니다. \n");
        return;
    }

    // --------------------
    // 1. 수평 이동속도 (m/s) + 입력 방향 (정규화, PhysX 기준)
    // --------------------
    PxVec3 velocity(0, 0, 0);
    if (wishDir.LengthSquared() > 0.0f)
    {
        PxVec3 dir(wishDir.x, 0, wishDir.z);
        dir.normalize();
        velocity.x = dir.x * m_MoveSpeed;
        velocity.z = dir.z * m_MoveSpeed;
    }


    // --------------------
    // 2. 지면 체크 
    // --------------------
    PxControllerState state;
    m_Controller->getState(state);
    bool isGrounded = state.collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN;


    // --------------------
    // 3. 점프 처리
    // --------------------
    if (isGrounded)
    {
        if (m_RequestJump)
        {
            m_VerticalVelocity = m_JumpSpeed;
            m_RequestJump = false;
        }
        else if (m_VerticalVelocity < 0.0f)
        {
            m_VerticalVelocity = m_MinDown;
        }
    }
    else
    {
        m_VerticalVelocity += -9.8f * fixedDt;
    }

    velocity.y = m_VerticalVelocity;


    // --------------------
    // 4. 이동 거리
    // --------------------
    PxVec3 move = velocity * fixedDt;


    // --------------------
    // 5. 필터
    // --------------------
    CCTQueryFilter queryFilter(nullptr); // 필요 시 자신 필터 설정
    PxControllerFilters filters(&m_FilterData, &queryFilter, nullptr);


    // --------------------
    // 6. 이동
    // --------------------
    m_Controller->move(move, 0.01f, fixedDt, filters);
}

void CharacterControllerComponent::Jump()
{
    if (!m_Controller || m_RequestJump)
        return;
    m_RequestJump = true;
}

void CharacterControllerComponent::SyncFromController()
{
    if (!transform || !m_Controller) return;

    PxExtendedVec3 p = m_Controller->getPosition();
    transform->SetPosition({
        (float)p.x * PHYSX_TO_WORLD - m_Offset.x,
        (float)p.y * PHYSX_TO_WORLD - m_Offset.y,
        (float)p.z * PHYSX_TO_WORLD - m_Offset.z
        });
    // 회전은 Transform 유지
}

void CharacterControllerComponent::SetLayer(CollisionLayer layer)
{
    m_FilterData.word0 = (uint32_t)layer;
    m_FilterData.word1 = PhysicsLayerMatrix::GetMask(layer);
    m_FilterData.word2 = 0;
    m_FilterData.word3 = 0;
}


// ----------------------------------
// 충돌 관리
// ----------------------------------

void CharacterControllerComponent::ResolveCollisions()
{
    // Enter / Stay
    for (auto* other : m_CCTCurrContacts)
    {
        if (m_CCTPrevContacts.find(other) == m_CCTPrevContacts.end())
        {
            OnCollisionEnter(other);
            other->OnCCTEnter(this);   // Physics에게 알림
        }
        else
        {
            OnCollisionStay(other);
            other->OnCCTStay(this);
        }
    }

    // Exit
    for (auto* other : m_CCTPrevContacts)
    {
        if (m_CCTCurrContacts.find(other) == m_CCTCurrContacts.end())
        {
            OnCollisionExit(other);
            other->OnCCTExit(this);
        }
    }

    // 다음 프레임 준비
    m_CCTPrevContacts = std::move(m_CCTCurrContacts);
    m_CCTCurrContacts.clear();
}

void CharacterControllerComponent::ResolveTriggers()
{
    // Enter / Stay
    for (auto* other : m_CCTCurrTriggers)
    {
        if (m_CCTPrevTriggers.find(other) == m_CCTPrevTriggers.end())
        {
            OnTriggerEnter(other);
            other->OnCCTEnter(this);
        }
        else
        {
            OnTriggerStay(other);
            other->OnCCTStay(this);
        }
    }

    // Exit
    for (auto* other : m_CCTPrevTriggers)
    {
        if (m_CCTCurrTriggers.find(other) == m_CCTCurrTriggers.end())
        {
            OnTriggerExit(other);
            other->OnCCTExit(this);
        }
    }

    m_CCTPrevTriggers = std::move(m_CCTCurrTriggers);
    m_CCTCurrTriggers.clear();
}


void CharacterControllerComponent::CheckTriggers()
{
    if (!m_Controller) return;

    PxScene* scene = PhysicsSystem::Instance().GetScene();
    PxCapsuleController* capsuleCtrl = static_cast<PxCapsuleController*>(m_Controller);

    // -------------------------------------------------
    // 1. CCT Capsule 정보
    // -------------------------------------------------
    const float radius = capsuleCtrl->getRadius();
    const float height = capsuleCtrl->getHeight();
    const float shrink = 0.01f;

    PxCapsuleGeometry capsule(
        PxMax(0.0f, radius - shrink),
        PxMax(0.0f, (height * 0.5f) - shrink)
    );

    // -------------------------------------------------
    // 2. CCT 위치 (PhysX 기준)
    // -------------------------------------------------
    PxExtendedVec3 p = m_Controller->getPosition();
    PxTransform pose(PxVec3((float)p.x, (float)p.y, (float)p.z));


    // -------------------------------------------------
    // 3. Overlap Query용 필터
    // -------------------------------------------------
    TriggerFilter filter(nullptr); 

    PxOverlapBufferN<64> hit;
    PxQueryFilterData qfd;
    qfd.data = m_FilterData;
    qfd.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC;

    scene->overlap(capsule, pose, hit, qfd, &filter);


    // -------------------------------------------------
    // 4. Trigger 수집
    // -------------------------------------------------
    m_CCTCurrTriggers.clear();
    for (PxU32 i = 0; i < hit.getNbAnyHits(); i++)
    {
        PhysicsComponent* comp = PhysicsSystem::Instance().GetComponent(hit.getAnyHit(i).actor);
        if (!comp) 
            continue;

        // Trigger가 아니면 수집하지 않음
        if (!comp->IsTrigger()) 
            continue; 

        // 레이어 필터 
        if (!PhysicsLayerMatrix::CanCollide(m_Layer, comp->GetLayer()))
            continue;

        m_CCTCurrTriggers.insert(comp);
    }
}

//void CharacterControllerComponent::ApplyFilter()
//{
//    // ----------------------------
//    // CCT 전용 Query Filter
//    // ----------------------------
//    if (m_Controller)
//    {
//        m_FilterData.word0 = (uint32_t)m_Layer;          // 자기 레이어
//        m_FilterData.word1 = PhysicsLayerMatrix::GetMask(m_Layer);
//        m_FilterData.word2 = 0;
//        m_FilterData.word3 = 0;
//    }
//}