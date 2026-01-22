#include "PhysicsComponent.h"
#include "../EngineSystem/PhysicsSystem.h"
#include "../EngineSystem/CharacterControllerSystem.h"
#include "../Util/PhysXUtils.h"
#include "../Util/PhysicsLayerMatrix.h"
#include "Transform.h"
#include <DirectXColors.h>
#include "../Object/GameObject.h"


void PhysicsComponent::OnInitialize()
{
    transform = GetOwner()->GetTransform();
}

PhysicsComponent::~PhysicsComponent()
{
    if (m_Actor)
    {
        PhysicsSystem::Instance().UnregisterComponent(m_Actor);
        PX_RELEASE(m_Actor);
    }
}


// ------------------------------
// 좌표 변환
// ------------------------------

// Transform → Physics  : Dynamic에는 매 프레임 쓰면 안 됨
void PhysicsComponent::SyncToPhysics()
{
    if (!m_Actor || !transform) return;

    PxTransform px;
    px.p = ToPx(transform->GetPosition());
    px.q = ToPxQuat(transform->GetQuaternion()); 

    m_Actor->setGlobalPose(px);
}

// Physics → Transform : 물리 시뮬 하고나서 매 프레임 실행
void PhysicsComponent::SyncFromPhysics()
{
    if (!transform) return;

    if (m_Actor) // 일반 액터 
    {
        PxTransform px = m_Actor->getGlobalPose();
        transform->SetPosition(ToDX(px.p));  
        transform->SetQuaternion(ToDXQuat(px.q)); 
    }
}

void PhysicsComponent::ApplyFilter()
{
    // ----------------------------
    // RigidActor / Shape 용
    // ----------------------------
    if (m_Shape)
    {
        PxFilterData data;
        data.word0 = (uint32_t)m_Layer;
        data.word1 = PhysicsLayerMatrix::GetMask(m_Layer);
        data.word2 = 0;
        data.word3 = 0;

        m_Shape->setSimulationFilterData(data);
        m_Shape->setQueryFilterData(data);
    }
}

void PhysicsComponent::SetLayer(CollisionLayer layer)
{
    m_Layer = layer;
    ApplyFilter();
}



// ------------------------------
// 외부 API 
// - density : 질량 
// ------------------------------

// Box 
void PhysicsComponent::CreateStaticBox(const Vector3& half, const Vector3& localOffset)
{
    ColliderDesc d;
    d.halfExtents = half;
    d.localOffset = localOffset;
    CreateCollider(ColliderType::Box, PhysicsBodyType::Static, d);
}
void PhysicsComponent::CreateTriggerBox(const Vector3& half, const Vector3& localOffset)
{
    ColliderDesc d;
    d.halfExtents = half;
    d.localOffset = localOffset;
    d.isTrigger = true;

    CreateCollider(ColliderType::Box, PhysicsBodyType::Static, d);
}
void PhysicsComponent::CreateDynamicBox(const Vector3& half, float density, const Vector3& localOffset)
{
    ColliderDesc d;
    d.halfExtents = half;
    d.density = density;
    d.localOffset = localOffset;
    CreateCollider(ColliderType::Box, PhysicsBodyType::Dynamic, d);
}


// Sphere 
void PhysicsComponent::CreateStaticSphere(float radius, const Vector3& localOffset)
{
    ColliderDesc d;
    d.radius = radius;
    d.localOffset = localOffset;
    CreateCollider(ColliderType::Sphere, PhysicsBodyType::Static, d);
}
void PhysicsComponent::CreateDynamicSphere(float radius, float density, const Vector3& localOffset)
{
    ColliderDesc d;
    d.radius = radius;
    d.density = density;
    d.localOffset = localOffset;
    CreateCollider(ColliderType::Sphere, PhysicsBodyType::Dynamic, d);
}

// Capsule 
void PhysicsComponent::CreateStaticCapsule(float radius, float height, const Vector3& localOffset)
{
    ColliderDesc d;
    d.radius = radius;
    d.height = height;
    d.localOffset = localOffset;
    CreateCollider(ColliderType::Capsule, PhysicsBodyType::Static, d);
}
void PhysicsComponent::CreateTriggerCapsule(float radius, float height, const Vector3& localOffset)
{
    ColliderDesc d;
    d.radius = radius;
    d.height = height;
    d.localOffset = localOffset;
    d.isTrigger = true;

    CreateCollider(ColliderType::Capsule, PhysicsBodyType::Static, d);
}
void PhysicsComponent::CreateDynamicCapsule(float radius, float height, float density, const Vector3& localOffset)
{
    ColliderDesc d;
    d.radius = radius;
    d.height = height;
    d.density = density;
    d.localOffset = localOffset;
    CreateCollider(ColliderType::Capsule, PhysicsBodyType::Dynamic, d);
}


// ------------------------------
// 내부 생성 
// ------------------------------
void PhysicsComponent::CreateCollider(ColliderType collider, PhysicsBodyType body, const ColliderDesc& d)
{
    auto& phys = PhysicsSystem::Instance();
    PxPhysics* px = phys.GetPhysics();
    PxMaterial* mat = phys.GetDefaultMaterial();

    m_IsTrigger = d.isTrigger;

    // ----------------------
    // Shape 생성
    // ----------------------
    PxTransform localPose;
    localPose.p = ToPx(d.localOffset);
    localPose.q = ToPxQuat(XMLoadFloat4(&d.localRotation));

    switch (collider)
    {
    case ColliderType::Box:
        m_Shape = px->createShape(PxBoxGeometry(d.halfExtents.x * WORLD_TO_PHYSX, d.halfExtents.y * WORLD_TO_PHYSX, d.halfExtents.z * WORLD_TO_PHYSX), *mat, true);
        break;

    case ColliderType::Sphere:
        m_Shape = px->createShape(PxSphereGeometry(d.radius * WORLD_TO_PHYSX), *mat, true);
        break;

    case ColliderType::Capsule:
        m_Shape = px->createShape(PxCapsuleGeometry(d.radius * WORLD_TO_PHYSX, (d.height * 0.5f) * WORLD_TO_PHYSX), *mat, true);

        PxQuat capsuleRot(PxHalfPi, PxVec3(0, 0, 1));// X축 캡슐 → Y축 캡슐로 회전 // Z축 +90도
        localPose.q = capsuleRot * localPose.q;
        break;
    }
    m_Shape->setLocalPose(localPose);


    // ----------------------
    // Shape Flag (Trigger / Simulation)
    // ----------------------
    if (d.isTrigger)
    {
        // Trigger는 충돌 계산 X
        m_Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
        m_Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
    }
    else
    {
        // 일반 Collider
        m_Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        m_Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
    }


    // ----------------------
    // Actor 생성
    // ----------------------
    if (body == PhysicsBodyType::Static || d.isTrigger)
    {
        // Trigger는 무조건 Static 
        m_Actor = px->createRigidStatic(PxTransform(PxIdentity));
    }
    else
    {
        PxRigidDynamic* dyn = px->createRigidDynamic(PxTransform(PxIdentity));

        if (body == PhysicsBodyType::Kinematic)
        {
            dyn->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        }

        m_Actor = dyn;
    }

    // ----------------------
    // Shape 연결
    // ----------------------
    m_Actor->attachShape(*m_Shape);


    // ----------------------
    // 질량 계산
    // ----------------------
    if (body == PhysicsBodyType::Dynamic)
    {
        PxRigidBodyExt::updateMassAndInertia( // 질량 계산 : Shape 부피 × density
            *static_cast<PxRigidDynamic*>(m_Actor),
            d.density
        );
    }
    
    phys.GetScene()->addActor(*m_Actor); // 물리 씬에 추가 
    phys.RegisterComponent(m_Actor, this);

    m_BodyType = body;
    m_ColliderType = collider;

    ApplyFilter(); // 레이어 필터 
}


DirectX::XMVECTOR GetActorDebugColor(PxRigidActor* actor)
{
    if (actor->is<PxRigidStatic>())
        return DirectX::Colors::Green;

    if (PxRigidDynamic* dyn = actor->is<PxRigidDynamic>())
    {
        if (dyn->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC)
            return DirectX::Colors::Magenta;

        return DirectX::Colors::Cyan;
    }

    return DirectX::Colors::White;
}

void PhysicsComponent::DrawPhysXActors()
{
    PxScene* scene = PhysicsSystem::Instance().GetScene();
    if (!scene) return;

    PxU32 actorCount = scene->getNbActors(
        PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC
    );

    std::vector<PxActor*> actors(actorCount);
    scene->getActors(
        PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC,
        actors.data(),
        actorCount
    );

    // [ Actor ] 
    for (PxActor* actor : actors)
    {
        PxRigidActor* rigid = actor->is<PxRigidActor>();
        if (!rigid) return;

        // CCT가 소유한 Actor는 제외 
        if (m_CCTActors.find(rigid) != m_CCTActors.end()) // if (m_CCTActors.contains(rigid)) 없음 
            continue;

        const XMVECTOR debugColor = GetActorDebugColor(rigid);
        PxTransform actorPose = rigid->getGlobalPose();

        PxU32 shapeCount = rigid->getNbShapes();
        std::vector<PxShape*> shapes(shapeCount);
        rigid->getShapes(shapes.data(), shapeCount);

        for (PxShape* shape : shapes)
        {
            // [ Shape ]
            DrawPhysXShape(shape, actorPose, debugColor);
        }
    }

    // Character Controller는 별도로 그려야 함 
    DrawCharacterControllers();
}

void PhysicsComponent::DrawPhysXShape(PxShape* shape, const PxTransform& actorPose, FXMVECTOR color)
{
    bool isTrigger = shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE;

    PxGeometryHolder geo = shape->getGeometry();
    PxTransform localPose = shape->getLocalPose();
    PxTransform worldPose = actorPose * localPose; // Actor * Shape Local

    switch (geo.getType())
    {
        // BOX
    case PxGeometryType::eBOX:
    {
        const PxBoxGeometry& box = geo.box();

        DirectX::BoundingOrientedBox obb;
        obb.Center = ToDX(worldPose.p);   // m -> cm
        obb.Extents = { PxToDX(box.halfExtents.x),PxToDX(box.halfExtents.y),PxToDX(box.halfExtents.z) };
        obb.Orientation = { worldPose.q.x,worldPose.q.y,worldPose.q.z,worldPose.q.w };

        DebugDraw::Draw(m_DebugBatch.get(), obb, color, isTrigger);
        break;
    }

    // SPHERE
    case PxGeometryType::eSPHERE:
    {
        const PxSphereGeometry& sphere = geo.sphere();

        DirectX::BoundingSphere bs;
        bs.Center = ToDX(worldPose.p);
        bs.Radius = PxToDX(sphere.radius);

        DebugDraw::Draw(m_DebugBatch.get(), bs, color, isTrigger);
        break;
    }

    // CAPSULE (PhysX X축 -> Render Y축)
    case PxGeometryType::eCAPSULE:
    {
        const PxCapsuleGeometry& capsule = geo.capsule();

        // PhysX 캡슐(X축)을 Render(Y축)으로 보정
        XMVECTOR physxToY = XMQuaternionRotationAxis(XMVectorSet(0, 0, 1, 0), XM_PIDIV2);

        XMVECTOR worldQ =
            XMVectorSet(worldPose.q.x, worldPose.q.y, worldPose.q.z, worldPose.q.w);

        XMVECTOR debugQ =
            XMQuaternionMultiply(physxToY, worldQ);

        PxQuat finalQ = ToPxQuat(debugQ);

        DebugDraw::DrawCapsule(
            m_DebugBatch.get(),
            PxVec3(
                worldPose.p.x * PHYSX_TO_WORLD,
                worldPose.p.y * PHYSX_TO_WORLD,
                worldPose.p.z * PHYSX_TO_WORLD
            ),
            capsule.radius * PHYSX_TO_WORLD,
            (capsule.halfHeight * 2.0f) * PHYSX_TO_WORLD,
            color,
            finalQ,
            isTrigger
        );
        break;
    }
    default:
        break;
    }
}

void PhysicsComponent::DrawCharacterControllers()
{
    PxControllerManager* mgr = CharacterControllerSystem::Instance().GetControllerManager();
    if (!mgr) return;

    PxU32 count = mgr->getNbControllers();

    for (PxU32 i = 0; i < count; ++i)
    {
        PxController* cct = mgr->getController(i);
        if (!cct) continue;

        if (cct->getType() != PxControllerShapeType::eCAPSULE)
            continue;

        PxCapsuleController* capsule = static_cast<PxCapsuleController*>(cct);

        PxExtendedVec3 p = capsule->getPosition();

        DebugDraw::DrawCapsule(
            m_DebugBatch.get(),
            PxVec3(
                (float)p.x * PHYSX_TO_WORLD,
                (float)p.y * PHYSX_TO_WORLD,
                (float)p.z * PHYSX_TO_WORLD
            ),
            capsule->getRadius() * PHYSX_TO_WORLD,
            capsule->getHeight() * PHYSX_TO_WORLD,
            DirectX::Colors::Red
        );
    }
}


// [ CCT Actor 수집 함수 ]
void PhysicsComponent::CollectCCTActors()
{
    m_CCTActors.clear();

    PxControllerManager* mgr = CharacterControllerSystem::Instance().GetControllerManager();
    if (!mgr) return;

    PxU32 count = mgr->getNbControllers();
    for (PxU32 i = 0; i < count; ++i)
    {
        PxController* cct = mgr->getController(i);
        if (!cct) continue;

        if (PxRigidActor* actor = cct->getActor())
        {
            m_CCTActors.insert(actor);
        }
    }
}




//RTTR_REGISTRATION
//{
//    rttr::registration::class_<PhysicsComponent>("PhysicsComponent")
//        .constructor<>()
//            (rttr::policy::ctor::as_std_shared_ptr)
//        .property("BodyType", &PhysicsComponent::GetBodyType, &PhysicsComponent::SetBodyType)
//        .property("ColliderType", &PhysicsComponent::GetColliderType, &PhysicsComponent::SetColliderType)
//        .property("HalfExtents", &PhysicsComponent::GetHalfExtents, &PhysicsComponent::SetHalfExtents)
//        .property("Radius", &PhysicsComponent::GetRadius, &PhysicsComponent::SetRadius)
//        .property("Height", &PhysicsComponent::GetHeight, &PhysicsComponent::SetHeight)
//        .property("Density", &PhysicsComponent::GetDensity, &PhysicsComponent::SetDensity)
//        .property("LocalOffset", &PhysicsComponent::GetLocalOffset, &PhysicsComponent::SetLocalOffset)
//        .property("Layer", &PhysicsComponent::GetLayer, &PhysicsComponent::SetLayer)
//        .property("IsTrigger", &PhysicsComponent::IsTrigger, &PhysicsComponent::SetTrigger);
//}
//
//
//
//nlohmann::json PhysicsComponent::Serialize()
//{
//    nlohmann::json datas;
//
//    rttr::type t = rttr::type::get(*this);
//    datas["type"] = t.get_name().to_string();
//    datas["properties"] = nlohmann::json::object();
//
//    for (auto& prop : t.get_properties())
//    {
//        std::string name = prop.get_name().to_string();
//        rttr::variant value = prop.get_value(*this);
//
//        if (value.is_type<PhysicsBodyType>())
//            datas["properties"][name] = (int)value.get_value<PhysicsBodyType>();
//        else if (value.is_type<ColliderType>())
//            datas["properties"][name] = (int)value.get_value<ColliderType>();
//        else if (value.is_type<Vector3>())
//        {
//            auto v = value.get_value<Vector3>();
//            datas["properties"][name] = { v.x, v.y, v.z };
//        }
//        else if (value.is_type<float>())
//            datas["properties"][name] = value.get_value<float>();
//        else if (value.is_type<bool>())
//            datas["properties"][name] = value.get_value<bool>();
//        else if (value.is_type<CollisionLayer>())
//            datas["properties"][name] = (int)value.get_value<CollisionLayer>();
//    }
//
//    return datas;
//}
//
//void PhysicsComponent::Deserialize(nlohmann::json data)
//{
//    auto propData = data["properties"];
//    rttr::type t = rttr::type::get(*this);
//
//    for (auto& prop : t.get_properties())
//    {
//        std::string name = prop.get_name().to_string();
//
//        if (!propData.contains(name)) continue;
//
//        if (prop.get_type() == rttr::type::get<PhysicsBodyType>())
//            prop.set_value(*this, (PhysicsBodyType)(int)propData[name]);
//        else if (prop.get_type() == rttr::type::get<ColliderType>())
//            prop.set_value(*this, (ColliderType)(int)propData[name]);
//        else if (prop.get_type() == rttr::type::get<Vector3>())
//        {
//            auto v = propData[name];
//            prop.set_value(*this, Vector3(v[0], v[1], v[2]));
//        }
//        else if (prop.get_type() == rttr::type::get<float>())
//            prop.set_value(*this, propData[name]);
//        else if (prop.get_type() == rttr::type::get<bool>())
//            prop.set_value(*this, propData[name]);
//        else if (prop.get_type() == rttr::type::get<CollisionLayer>())
//            prop.set_value(*this, (CollisionLayer)(int)propData[name]);
//    }
//
//    RebuildPhysics(); // 복원 후 재생성
//}


// ------------------------------
// 콜라이더 생성 
// ------------------------------
//void PhysicsComponent::RebuildPhysics()
//{
//    auto& phys = PhysicsSystem::Instance();
//    PxPhysics* px = phys.GetPhysics();
//    PxMaterial* mat = phys.GetDefaultMaterial();
//
//    // 기존 제거
//    if (m_Actor)
//    {
//        phys.UnregisterComponent(m_Actor);
//        PX_RELEASE(m_Actor);
//        m_Actor = nullptr;
//        m_Shape = nullptr;
//    }
//
//
//    // ----------------------
//    // Shape 생성
//    // ----------------------
//    PxTransform localPose;
//    localPose.p = ToPx(m_LocalOffset);
//    localPose.q = PxQuat(PxIdentity);
//
//    switch (m_ColliderType)
//    {
//    case ColliderType::Box:
//        m_Shape = px->createShape(PxBoxGeometry(m_HalfExtents.x * WORLD_TO_PHYSX, m_HalfExtents.y * WORLD_TO_PHYSX, m_HalfExtents.z * WORLD_TO_PHYSX), *mat, true);
//        break;
//
//    case ColliderType::Sphere:
//        m_Shape = px->createShape(PxSphereGeometry(m_Radius * WORLD_TO_PHYSX), *mat, true);
//        break;
//
//    case ColliderType::Capsule:
//        m_Shape = px->createShape(PxCapsuleGeometry(m_Radius * WORLD_TO_PHYSX, (m_Height * 0.5f) * WORLD_TO_PHYSX), *mat, true);
//
//        // X축 캡슐 → Y축 : Z축 +90도
//        {
//            PxQuat capsuleRot(PxHalfPi, PxVec3(0, 0, 1));
//            localPose.q = capsuleRot;
//        }
//        break;
//    }
//
//    m_Shape->setLocalPose(localPose);
//
//
//    // ----------------------
//    // Shape Flag (Trigger / Simulation)
//    // ----------------------
//    if (m_IsTrigger)
//    {
//        // Trigger는 충돌 계산 X
//        m_Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
//        m_Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
//    }
//    else
//    {
//        // 일반 Collider 
//        m_Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
//        m_Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
//    }
//
//    // ----------------------
//    // Actor 생성
//    // ----------------------
//    PxTransform globalPose;
//    globalPose.p = ToPx(transform->GetPosition());
//    globalPose.q = ToPxQuat(transform->GetQuaternion());
//
//    if (m_BodyType == PhysicsBodyType::Static || m_IsTrigger)
//    {
//        m_Actor = px->createRigidStatic(globalPose); // Trigger는 무조건 Static 
//    }
//    else
//    {
//        PxRigidDynamic* dyn = px->createRigidDynamic(globalPose);
//
//        if (m_BodyType == PhysicsBodyType::Kinematic)
//        {
//            dyn->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
//        }
//
//        m_Actor = dyn;
//    }
//
//    // ----------------------
//    // Shape 연결
//    // ----------------------
//    m_Actor->attachShape(*m_Shape);
//
//
//    // ----------------------
//    // 질량 계산
//    // ----------------------
//    if (m_BodyType == PhysicsBodyType::Dynamic)
//    {
//        PxRigidBodyExt::updateMassAndInertia(*static_cast<PxRigidDynamic*>(m_Actor), m_Density);
//    }
//
//
//    // ----------------------
//    // Layer Filter
//    // ----------------------
//    ApplyFilter();
//
//
//    // ----------------------
//    // Scene 등록
//    // ----------------------
//    phys.GetScene()->addActor(*m_Actor);
//    phys.RegisterComponent(m_Actor, this);
//}