#pragma once
#include <PxPhysicsAPI.h>
#include <directxtk/SimpleMath.h>
#include <unordered_set>

#include "../Util/CollisionLayer.h"
#include "../Object/Component.h"

using namespace DirectX::SimpleMath;
using namespace physx;

class Transform;
class PhysicsComponent;

class CharacterControllerComponent : public Component
{
    RTTR_ENABLE(CharacterControllerComponent)
//public: // TODO : 직렬화 역직렬화 
//
//    nlohmann::json Serialize() override;
//    void Deserialize(nlohmann::json data) override;

public:
    void OnInitialize() override;

public:
    Transform* transform = nullptr;

    // CCT 전용 충돌 상태 (프레임 비교)
    std::unordered_set<PhysicsComponent*> m_CCTPrevContacts;
    std::unordered_set<PhysicsComponent*> m_CCTCurrContacts;

    // CCT 전용 Trigger 상태
    std::unordered_set<PhysicsComponent*> m_CCTPrevTriggers;
    std::unordered_set<PhysicsComponent*> m_CCTCurrTriggers;


public:
    PxController* m_Controller = nullptr;
    Vector3 m_Offset = { 0,0,0 };       // CCT 전용 오프셋

private:
    const float m_MinDown = -1.0f;

    float m_VerticalVelocity = 0.0f;
    bool m_RequestJump = false;
    float m_JumpSpeed = 5.5f;
    float m_MoveSpeed = 2.0f;
   
    PxFilterData m_FilterData;

    CollisionLayer m_Layer = CollisionLayer::Default;
    CollisionMask  m_Mask = 0xFFFFFFFF;
    bool m_IsTrigger = false;


public:
    // -----------------------------
    // Collision / Trigger 이벤트 콜백 
    // -----------------------------
    virtual void OnCollisionEnter(PhysicsComponent* other) { OutputDebugStringW(L"[CharacterControllerComponent] Collision Enter! \n"); }
    virtual void OnCollisionStay(PhysicsComponent* other) { /*OutputDebugStringW(L"[CharacterControllerComponent] Collision Stay! \n");*/ }
    virtual void OnCollisionExit(PhysicsComponent* other) { OutputDebugStringW(L"[CharacterControllerComponent] Collision Exit! \n"); }

    virtual void OnTriggerEnter(PhysicsComponent* other) { OutputDebugStringW(L"[CharacterControllerComponent] Trigger Enter! \n"); }
    virtual void OnTriggerStay(PhysicsComponent* other) { /*OutputDebugStringW(L"[CharacterControllerComponent] Trigger Stay \n");*/ }
    virtual void OnTriggerExit(PhysicsComponent* other) { OutputDebugStringW(L"[CharacterControllerComponent] Trigger Exit! \n"); }


public:
    bool IsTrigger() const { return m_IsTrigger; }



public:
    CharacterControllerComponent() = default;
    ~CharacterControllerComponent();

    void CreateCharacterCollider(float radius, float height, const Vector3& offset);
    void MoveCharacter(const Vector3& wishDir, float fixedDt);
    void Jump();

    void SyncFromController();

    void ResolveCollisions();
    void ResolveTriggers();
    void CheckTriggers();

    // --------------------------
    // 충돌 레이어 
    // --------------------------
    void SetLayer(CollisionLayer layer);
    CollisionLayer GetLayer() const { return m_Layer; }

private:
    // void ApplyFilter();
};
