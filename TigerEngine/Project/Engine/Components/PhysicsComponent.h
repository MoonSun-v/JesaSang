#pragma once
#include <PxPhysicsAPI.h>
#include <memory>
#include <directxtk/SimpleMath.h>
#include <unordered_set>

#include "../Util/CollisionLayer.h"
#include "../Object/Component.h"
#include "../Util/DebugDraw.h"

using namespace DirectX::SimpleMath;
using namespace physx;

class Transform;
class CharacterControllerComponent;

// ------------------------------
// Enum (엔진 내부용)
// ------------------------------
enum class PhysicsBodyType
{
    Static,
    Dynamic,
    Kinematic
};

enum class ColliderType
{
    Box,
    Sphere,
    Capsule
};


// ------------------------------
// Collider Desc
// ------------------------------
struct ColliderDesc
{
    Vector3 halfExtents = { 50,50,50 };// Box
    float radius = 10.0f;            // Sphere / Capsule
    float height = 10.0f;            // Capsule
    float density = 10.0f;           // Dynamic

    Vector3 localOffset = { 0, 0, 0 };
    Quaternion localRotation = Quaternion::Identity; // 회전 아직 적용X 사용하려나..? 

    bool isTrigger = false;
};


// ----------------------------------------------------
// [ [엔진 Transform]  ←→  [PhysX RigidActor] ]
// - 엔진 쪽 Transform (position / rotation) 을 가지고 있는 GameObject(owner) 와 
// - PhysX 쪽 PxRigidActor(m_Actor)
//   이 둘을 양방향으로 동기화 하는 역할 
// ----------------------------------------------------
class PhysicsComponent : public Component
{
    RTTR_ENABLE(PhysicsComponent)
//public: // TODO : 직렬화 역직렬화 
//
//    nlohmann::json Serialize() override;
//    void Deserialize(nlohmann::json data) override;

public:
    void OnInitialize() override;

public:
    Transform* transform = nullptr;

    // Collision / Trigger 이벤트 관리 
    std::unordered_set<PhysicsComponent*> m_CollisionActors;  // 현재 접촉 Actor
    std::unordered_set<PhysicsComponent*> m_TriggerActors;    // 현재 Trigger Actor
    std::unordered_set<PhysicsComponent*> m_PendingTriggers; // Trigger 수집용

    bool IsTrigger() const { return m_IsTrigger; }


private:
    // [ 직렬화 대상 ]
    PhysicsBodyType m_BodyType = PhysicsBodyType::Static;
    ColliderType m_ColliderType = ColliderType::Box;

    Vector3 m_HalfExtents = { 10,10,10 };
    float m_Radius = 50.0f;
    float m_Height = 10.0f;
    float m_Density = 10.0f;
    Vector3 m_LocalOffset = { 0,0,0 };

    CollisionLayer m_Layer = CollisionLayer::Default;
    bool m_IsTrigger = false;


    // [ 런타임 전용 ]
    PxRigidActor* m_Actor = nullptr;
    PxShape* m_Shape = nullptr;

    std::unordered_set<PxRigidActor*> m_CCTActors;

public:
    PhysicsComponent() = default;
    ~PhysicsComponent();

    // -----------------------------
    // Collision / Trigger 이벤트 콜백 
    // -----------------------------
    virtual void OnCollisionEnter(PhysicsComponent* other) { OutputDebugStringW(L"[PhysicsComponent] Collision Enter! \n"); }
    virtual void OnCollisionStay(PhysicsComponent* other) { /*OutputDebugStringW(L"[PhysicsComponent] Collision Stay! \n");*/ }
    virtual void OnCollisionExit(PhysicsComponent* other) { OutputDebugStringW(L"[PhysicsComponent] Collision Exit! \n"); }

    virtual void OnTriggerEnter(PhysicsComponent* other) { OutputDebugStringW(L"[PhysicsComponent] Trigger Enter! \n"); }
    virtual void OnTriggerStay(PhysicsComponent* other) { /*OutputDebugStringW(L"[PhysicsComponent] Trigger Stay \n");*/ }
    virtual void OnTriggerExit(PhysicsComponent* other) { OutputDebugStringW(L"[PhysicsComponent] Trigger Exit! \n"); }

    // CCT 이벤트
    virtual void OnCCTEnter(CharacterControllerComponent* cct) {}
    virtual void OnCCTStay(CharacterControllerComponent* cct) {}
    virtual void OnCCTExit(CharacterControllerComponent* cct) {}


    // --------------------------
    // 외부 API : Collider / Actor 생성 
    // --------------------------
    void CreateStaticBox(const Vector3& half, const Vector3& localOffset = { 0,0,0 });
    void CreateTriggerBox(const Vector3& half, const Vector3& localOffset = { 0,0,0 }); // Trigger는 Static Actor가 정석
    void CreateDynamicBox(const Vector3& half, float density = 1.0f, const Vector3& localOffset = { 0,0,0 });

    void CreateStaticSphere(float radius, const Vector3& localOffset = { 0,0,0 });
    void CreateDynamicSphere(float radius, float density = 1.0f, const Vector3& localOffset = { 0,0,0 });

    void CreateStaticCapsule(float radius, float height, const Vector3& localOffset = { 0,0,0 });
    void CreateTriggerCapsule(float radius, float height, const Vector3& localOffset = { 0,0,0 });
    void CreateDynamicCapsule(float radius, float height, float density = 1.0f, const Vector3& localOffset = { 0,0,0 });


    // --------------------------
    // Transform 연동
    // --------------------------
    void SyncToPhysics();
    void SyncFromPhysics();

    // --------------------------
    // 충돌 레이어 
    // --------------------------
    void SetLayer(CollisionLayer layer);
    CollisionLayer GetLayer() const { return m_Layer; }

private:
    void ApplyFilter();
    // void RebuildPhysics();

    // 유틸 
    void DrawPhysXActors();
    void DrawPhysXShape(PxShape* shape, const PxTransform& actorPose, FXMVECTOR color);
    void DrawCharacterControllers();
    void CollectCCTActors();

    std::unique_ptr<PrimitiveBatch<VertexPositionColor>> m_DebugBatch;

private:
    // --------------------------
    // 내부 생성기
    // --------------------------
    void CreateCollider(ColliderType collider, PhysicsBodyType body, const ColliderDesc& desc);


//public: 
//    // Getter / Setter (RTTR 전용)
//
//    PhysicsBodyType GetBodyType() const { return m_BodyType; }
//    void SetBodyType(PhysicsBodyType t) { m_BodyType = t; RebuildPhysics(); }
//
//    ColliderType GetColliderType() const { return m_ColliderType; }
//    void SetColliderType(ColliderType t) { m_ColliderType = t; RebuildPhysics(); }
//
//    Vector3 GetHalfExtents() const { return m_HalfExtents; }
//    void SetHalfExtents(Vector3 v) { m_HalfExtents = v; RebuildPhysics(); }
//
//    float GetRadius() const { return m_Radius; }
//    void SetRadius(float v) { m_Radius = v; RebuildPhysics(); }
//
//    float GetHeight() const { return m_Height; }
//    void SetHeight(float v) { m_Height = v; RebuildPhysics(); }
//
//    float GetDensity() const { return m_Density; }
//    void SetDensity(float v) { m_Density = v; RebuildPhysics(); }
//
//    Vector3 GetLocalOffset() const { return m_LocalOffset; }
//    void SetLocalOffset(Vector3 v) { m_LocalOffset = v; RebuildPhysics(); }
//
//    // CollisionLayer GetLayer() const { return m_Layer; }
//    // void SetLayer(CollisionLayer v) { m_Layer = v; ApplyFilter(); }
//
//    // bool IsTrigger() const { return m_IsTrigger; }
//    void SetTrigger(bool v) { m_IsTrigger = v; RebuildPhysics(); }

};