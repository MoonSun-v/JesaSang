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
    RTTR_ENABLE(Component)
public: 

    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

public:
    void OnInitialize() override;
    void OnStart() override; 
    void OnDestory() override;

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

private:
    const float m_MinDown = -1.0f;

    float m_VerticalVelocity = 0.0f;
    bool m_RequestJump = false;

    PxFilterData m_FilterData;

public:
    // 직렬화 대상 
    float m_Radius = 30.0f;
    float m_Height = 120.0f;
    Vector3 m_Offset;

    float m_JumpSpeed = 5.5f;
    float m_MoveSpeed = 2.0f;

    CollisionLayer m_Layer;
    CollisionMask  m_Mask;
    bool m_IsTrigger;



public:
    // -----------------------------
    // Collision / Trigger 이벤트 콜백 
    // -----------------------------
    virtual void OnCollisionEnter(PhysicsComponent* other);
    virtual void OnCollisionStay(PhysicsComponent* other);
    virtual void OnCollisionExit(PhysicsComponent* other);

    virtual void OnTriggerEnter(PhysicsComponent* other);
    virtual void OnTriggerStay(PhysicsComponent* other);
    virtual void OnTriggerExit(PhysicsComponent* other);

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

    void Teleport(const Vector3& pos);

private:
    // void ApplyFilter();
};
