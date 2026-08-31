#include "CharacterControllerComponent.h"
#include "../EngineSystem/CharacterControllerSystem.h"
#include "../EngineSystem/PhysicsSystem.h"
#include "../Util/PhysicsLayerMatrix.h"
#include "../Util/PhysXUtils.h"
#include "../Object/GameObject.h"

#include "Transform.h"
#include "PhysicsComponent.h"
#include "../Util/JsonHelper.h"


RTTR_REGISTRATION
{
    rttr::registration::enumeration<CollisionLayer>("CollisionLayer")
        (
            rttr::value("Default", CollisionLayer::Default),
            rttr::value("Player", CollisionLayer::Player),
            rttr::value("Enemy", CollisionLayer::Enemy),
            rttr::value("World", CollisionLayer::World),
            rttr::value("Trigger", CollisionLayer::Trigger),
            rttr::value("Projectile", CollisionLayer::Projectile),
            rttr::value("Vision", CollisionLayer::Vision),
            rttr::value("Ground", CollisionLayer::Ground)
            );

    rttr::registration::class_<CharacterControllerComponent>("CharacterControllerComponent")
        .constructor<>()
        .property("radius", &CharacterControllerComponent::m_Radius)
        .property("height", &CharacterControllerComponent::m_Height)
        .property("offset", &CharacterControllerComponent::m_Offset)

        .property("jumpSpeed", &CharacterControllerComponent::m_JumpSpeed)
        .property("moveSpeed", &CharacterControllerComponent::m_MoveSpeed)

        .property("layer", &CharacterControllerComponent::m_Layer)
        .property("isTrigger", &CharacterControllerComponent::m_IsTrigger);
}

nlohmann::json CharacterControllerComponent::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void CharacterControllerComponent::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);

    // data 수정 ...
    if (!m_Controller)
    {
        CreateCharacterCollider(m_Radius, m_Height, m_Offset);
    }

    CharacterControllerSystem::Instance().RegisterComponent(this, m_Controller);

    m_firstRegister = true;
}

void CharacterControllerComponent::OnCollisionEnter(PhysicsComponent* other) { if (GetOwner()) GetOwner()->BroadcastCollisionEnter(other); }
void CharacterControllerComponent::OnCollisionStay(PhysicsComponent* other) { if (GetOwner()) GetOwner()->BroadcastCollisionStay(other); }
void CharacterControllerComponent::OnCollisionExit(PhysicsComponent* other) { if (GetOwner()) GetOwner()->BroadcastCollisionExit(other); }

void CharacterControllerComponent::OnTriggerEnter(PhysicsComponent* other) { if (GetOwner()) GetOwner()->BroadcastTriggerEnter(other); }
void CharacterControllerComponent::OnTriggerStay(PhysicsComponent* other) { if (GetOwner())GetOwner()->BroadcastTriggerStay(other); }
void CharacterControllerComponent::OnTriggerExit(PhysicsComponent* other) { if (GetOwner()) GetOwner()->BroadcastTriggerExit(other); }

void CharacterControllerComponent::OnCCTCollisionEnter(CharacterControllerComponent* other) { if (GetOwner()) GetOwner()->BroadcastCCTCollisionEnter(other); }
void CharacterControllerComponent::OnCCTCollisionStay(CharacterControllerComponent* other) { if (GetOwner()) GetOwner()->BroadcastCCTCollisionStay(other); }
void CharacterControllerComponent::OnCCTCollisionExit(CharacterControllerComponent* other) { if (GetOwner()) GetOwner()->BroadcastCCTCollisionExit(other); }


void CharacterControllerComponent::OnInitialize()
{
    transform = GetOwner()->GetTransform();
}

void CharacterControllerComponent::OnStart()
{
}

void CharacterControllerComponent::OnDestory()
{
    
}

void CharacterControllerComponent::Enable_Inner()
{
    if(m_firstRegister)
    {
        CreateCharacterCollider(m_Radius, m_Height, m_Offset);
        // SetLayer(m_Layer);
        CharacterControllerSystem::Instance().RegisterComponent(this, m_Controller);
    }

    OnEnable();
}

void CharacterControllerComponent::Disable_Inner()
{
    if (m_Controller)
    {
        CharacterControllerSystem::Instance().UnRegisterComponent(this);
        m_Controller = nullptr;
        OnDisable();
    }
}


CharacterControllerComponent::~CharacterControllerComponent()
{
}

void CharacterControllerComponent::CreateCharacterCollider(float radius, float height, const Vector3& offset)
{
    if (!transform) return;
    CharacterControllerSystem::Instance().UnRegisterComponent(this); // 혹시 남아있다면 제거 

    m_Radius = radius;
    m_Height = height;
    m_Offset = offset;

    PxExtendedVec3 pos(
        (transform->GetLocalPosition().x + offset.x) * WORLD_TO_PHYSX,
        (transform->GetLocalPosition().y + offset.y) * WORLD_TO_PHYSX,
        (transform->GetLocalPosition().z + offset.z) * WORLD_TO_PHYSX
    );

    m_Controller = CharacterControllerSystem::Instance().CreateCapsuleCollider(
        pos,
        radius * WORLD_TO_PHYSX,
        height * WORLD_TO_PHYSX,
        10.0f   // density (사실상 무의미) density는 반드시 > 0
    );

     SetLayer(m_Layer); // 레이어 적용
}

//void CharacterControllerComponent::MoveCharacter(const Vector3& wishDir, float fixedDt)
//{
//    if (!m_Controller) // 방어코드 !! 유니티 내부도 이렇게 방어 한다고 함 
//    {
//        return;
//    }
//
//    auto& sys = CharacterControllerSystem::Instance();
//    sys.GetHitReport().owner = this;
//
//    // --------------------
//    // 1. 수평 이동속도 (m/s) + 입력 방향 (정규화, PhysX 기준)
//    // --------------------
//    PxVec3 velocity(0, 0, 0);
//    if (wishDir.LengthSquared() > 0.0f)
//    {
//        PxVec3 dir(wishDir.x, 0, wishDir.z);
//        dir.normalize();
//        velocity.x = dir.x * m_MoveSpeed;
//        velocity.z = dir.z * m_MoveSpeed;
//    }
//
//
//    // --------------------
//    // 2. 지면 체크 
//    // --------------------
//    PxControllerState state;
//    m_Controller->getState(state);
//    bool isGrounded = state.collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN;
//
//
//    // --------------------
//    // 3. 점프 처리
//    // --------------------
//    if (isGrounded)
//    {
//        if (m_RequestJump)
//        {
//            m_VerticalVelocity = m_JumpSpeed;
//            m_RequestJump = false;
//        }
//        else if (m_VerticalVelocity < 0.0f)
//        {
//            m_VerticalVelocity = m_MinDown;
//        }
//    }
//    else
//    {
//        m_VerticalVelocity += -9.8f * fixedDt;
//    }
//
//    velocity.y = m_VerticalVelocity;
//
//
//    // --------------------
//    // 4. 이동 거리
//    // --------------------
//    PxVec3 move = velocity * fixedDt;
//
//
//    // --------------------
//    // 5. 필터
//    // --------------------
//    CCTQueryFilter queryFilter(nullptr); // 필요 시 자신 필터 설정
//    PxControllerFilters filters(&m_FilterData, &queryFilter, nullptr);
//
//
//    // --------------------
//    // 6. 이동
//    // --------------------
//    m_Controller->move(move, 0.01f, fixedDt, filters);
//}

void CharacterControllerComponent::MovePlayer(const Vector3& inputDir, float playerSpeed, float dt)
{
    InternalMove(inputDir, playerSpeed, true, true, dt);
}
void CharacterControllerComponent::MoveAI( const Vector3& moveDir, float aiSpeed, float dt)
{
    InternalMove(moveDir, aiSpeed, true, false, dt);
}


// [ 공통 코어 움직임 ]
void CharacterControllerComponent::InternalMove(
    const Vector3& horizontalDir,
    float speed,           
    bool applyGravity,
    bool allowJump,
    float fixedDt)
{
    if (!m_Controller) return;  // 방어코드 !! 유니티 내부도 이렇게 방어 한다고 함 

    PxVec3 velocity(0, 0, 0);

    if (horizontalDir.LengthSquared() > 0.0f)
    {
        Vector3 d = horizontalDir;
        d.Normalize();
        velocity.x = d.x * speed;
        velocity.z = d.z * speed;
    }

    PxControllerState state;
    m_Controller->getState(state);
    bool isGrounded = state.collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN;

    if (applyGravity)
    {
        if (isGrounded)
        {
            if (allowJump && m_RequestJump)
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
    }
    else
    {
        velocity.y = 0.0f;
    }

    PxVec3 move = velocity * fixedDt;

    CCTQueryFilter queryFilter(nullptr);
    PxControllerFilters filters(&m_FilterData, &queryFilter, nullptr);

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
            other->OnCCTCollisionEnter(this);   // Physics에게 알림
        }
        else
        {
            OnCollisionStay(other);
            other->OnCCTCollisionStay(this);
        }
    }

    // Exit
    for (auto* other : m_CCTPrevContacts)
    {
        if (m_CCTCurrContacts.find(other) == m_CCTCurrContacts.end())
        {
            OnCollisionExit(other);
            other->OnCCTCollisionExit(this);
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
            other->OnCCTTriggerEnter(this);
        }
        else
        {
            OnTriggerStay(other);
            other->OnCCTTriggerStay(this);
        }
    }

    // Exit
    for (auto* other : m_CCTPrevTriggers)
    {
        if (m_CCTCurrTriggers.find(other) == m_CCTCurrTriggers.end())
        {
            OnTriggerExit(other);
            other->OnCCTTriggerExit(this);
        }
    }

    // 현재 결과를 다음 Fixed Step의 비교 기준으로 저장
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

    // PxCapsuleGeometry는 로컬 X축을 길이축으로 사용한다.
    // CCT의 기본 Up Direction인 Y축과 일치하도록 Z축을 기준으로 90도 회전한다.
    const PxQuat capsuleRotation(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f));
    PxTransform pose(
        PxVec3((float)p.x, (float)p.y, (float)p.z),
        capsuleRotation
    );


    // -------------------------------------------------
    // 3. Overlap Query용 필터
    // -------------------------------------------------
    // 자기 자신을 제외하고 Layer와 Mask를 검사할 수 있도록 현재 Component를 전달한다.
    TriggerFilter filter(this);

    PxOverlapBufferN<64> hit;
    PxQueryFilterData qfd;
    qfd.data = m_FilterData;
    // ePREFILTER가 있어야 Overlap Query 과정에서 TriggerFilter::preFilter()가 호출된다.
    qfd.flags = PxQueryFlag::eSTATIC |
                PxQueryFlag::eDYNAMIC |
                PxQueryFlag::ePREFILTER;

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

void CharacterControllerComponent::ResolveCCTCollisions()
{
    // Enter / Stay
    for (auto* other : m_CCTCurrCCTContacts)
    {
        if (m_CCTPrevCCTContacts.find(other) == m_CCTPrevCCTContacts.end())
        {
            OnCCTCollisionEnter(other);
            other->OnCCTCollisionEnter(this);
        }
        else
        {
            OnCCTCollisionStay(other);
            other->OnCCTCollisionStay(this);
        }
    }

    // Exit
    for (auto* other : m_CCTPrevCCTContacts)
    {
        if (m_CCTCurrCCTContacts.find(other) == m_CCTCurrCCTContacts.end())
        {
            OnCCTCollisionExit(other);
            other->OnCCTCollisionExit(this);
        }
    }

    // 다음 프레임 준비
    m_CCTPrevCCTContacts = std::move(m_CCTCurrCCTContacts);
    m_CCTCurrCCTContacts.clear();
}


void CharacterControllerComponent::Teleport(const Vector3& pos)
{
    PxExtendedVec3 p(
        pos.x * WORLD_TO_PHYSX + m_Offset.x,
        pos.y * WORLD_TO_PHYSX + m_Offset.y,
        pos.z * WORLD_TO_PHYSX + m_Offset.z
    );
    m_Controller->setPosition(p);
}
