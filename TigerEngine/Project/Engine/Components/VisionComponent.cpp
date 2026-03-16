#include "VisionComponent.h"
#include "../Util/JsonHelper.h"
#include "../EngineSystem/PhysicsSystem.h"
#include "../Components/Transform.h"
#include "../Object/GameObject.h"
#include "../Util/PhysXUtils.h"
#include "../Util/PhysicsLayerMatrix.h"


static float Deg2Rad(float deg)
{
    return deg * 3.1415926535f / 180.0f;
}

RTTR_REGISTRATION
{
    rttr::registration::class_<VisionComponent>("VisionComponent")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}
nlohmann::json VisionComponent::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}
void VisionComponent::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void VisionComponent::Enable_Inner()
{
    ScriptSystem::Instance().Register(this);
}

void VisionComponent::Disable_Inner()
{
    ScriptSystem::Instance().UnRegister(this);
}

// -----------------------------------------------------------

// [ 타겟이 시야 내에 있는지 확인 ]
bool VisionComponent::CheckVision(GameObject* target, float fov, float maxDistance)
{
    if (!target) return false;

    m_LastDebug.fov = fov;
    m_LastDebug.dist = maxDistance;
    m_LastDebug.valid = true;

    auto* selfTr = GetOwner()->GetTransform();
    auto* targetTr = target->GetTransform();

    // Ray origin (world)
    Vector3 origin = selfTr->GetWorldPosition();
    origin.y += eyeHigh;

    // target pos는 "local" 
    Vector3 targetPos = targetTr->GetLocalPosition();

    // 3D 방향/거리 (Raycast용)
    Vector3 toTarget3D = targetPos - origin;
    float dist3D = toTarget3D.Length();
    if (dist3D <= 0.0001f) return true;      // 거의 같은 위치면 보인다고 처리(선택)
    if (dist3D > maxDistance) return false;

    Vector3 dir3D = toTarget3D;
    dir3D.Normalize();

    // 2D(XZ) 방향 (FOV용)
    Vector3 forward = - selfTr->GetForward();
    Vector3 flatForward = forward; flatForward.y = 0.0f;
    if (flatForward.Length() <= 0.0001f) return false;
    flatForward.Normalize();

    Vector3 flatToTarget = toTarget3D; flatToTarget.y = 0.0f;
    if (flatToTarget.Length() <= 0.0001f)
    {
        // 타겟이 거의 바로 위/아래라면: FOV를 통과시키거나(보통 통과) 원하는 정책으로
        // 여기서는 통과 처리
    }
    else
    {
        flatToTarget.Normalize();
        float dot = flatForward.Dot(flatToTarget);
        float limit = cosf(Deg2Rad(fov * 0.5f));
        if (dot < limit) return false;
    }

    // PhysX Raycast: 타겟까지 거리만 쏘는 게 일반적으로 더 정확
    PxVec3 originPx = ToPx(origin);
    PxVec3 dirPx(dir3D.x, dir3D.y, dir3D.z);
    float maxDistPx = dist3D; // <= 핵심 변경

    std::vector<RaycastHit> hits;
    if (!PhysicsSystem::Instance().Raycast(
        originPx,
        dirPx,
        maxDistPx,
        hits,
        CollisionLayer::Default,
        QueryTriggerInteraction::Ignore,
        true))
    {
        return false;
    }

    std::sort(hits.begin(), hits.end(),
        [](const RaycastHit& a, const RaycastHit& b) { return a.distance < b.distance; });

    PhysicsComponent* firstComp = nullptr;
    for (auto& hit : hits)
    {
        if (!hit.component) continue;

        // 자기 자신 무시 (중요)
        if (hit.component->GetOwner() == GetOwner())
            continue;

        bool isTrigger = hit.shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE;
        if (isTrigger) continue;

        if (!PhysicsLayerMatrix::CanCollide(CollisionLayer::Default, hit.component->GetLayer()))
            continue;

        //std::cout << "  Object: " << hit.component->GetOwner()->GetName()
        //    << " | Layer: " << (uint32_t)hit.component->GetLayer()
        //    << " | Distance: " << hit.distance
        //    << std::endl;

        firstComp = hit.component;
        break;
    }

    //if (firstComp)
    //{
    //    auto* obj = firstComp->GetOwner();
    //    std::cout << "[Vision] Ray hit -> Object: " << obj->GetName()
    //        << " | Layer: " << (uint32_t)firstComp->GetLayer()
    //        << std::endl;
    //}

    return firstComp && firstComp->GetOwner() == target;
}


// [ 시야 디버그 그리기 ]
void VisionComponent::DrawDebugVision()
{
    if (!m_LastDebug.valid) return;

    auto* tr = GetOwner()->GetTransform();
    Vector3 origin = tr->GetWorldPosition();
    origin.y += eyeHigh;               // CheckVision과 동일한 눈 위치 
    Vector3 forward = - tr->GetForward();
    Vector3 right = tr->GetRight();

    // PhysX 변환
    PxVec3 originPx = ToPx(origin);
    float maxDistPx = m_LastDebug.dist;

    const int SEGMENTS = 32; // 시야 원 분할 수
    float halfRad = Deg2Rad(m_LastDebug.fov * 0.5f);

    XMVECTOR o = XMVectorSet(origin.x, origin.y + 5.0f, origin.z, 1);

    // FOV 원 시뮬레이션
    for (int i = 0; i <= SEGMENTS; ++i)
    {
        float t = -halfRad + (halfRad * 2.f) * (float(i) / SEGMENTS);

        Vector3 dir = forward * cosf(t) + right * sinf(t);
        dir.y = 0;              // FOV 평면 기준
        dir.Normalize();

        PxVec3 dirPx(dir.x, dir.y, dir.z);

        std::vector<RaycastHit> hits;
        float drawDist = m_LastDebug.dist;

        if (PhysicsSystem::Instance().Raycast(
            originPx,
            dirPx,
            maxDistPx,
            hits,
            CollisionLayer::Default,
            QueryTriggerInteraction::Ignore,
            false // BLOCK 하나
        ))
        {
            // 길이 조절
            drawDist = hits[0].distance;
        }

        XMVECTOR end = o + XMVectorSet(dir.x, dir.y, dir.z, 0) * drawDist;

        DebugDraw::DrawRayDebug(
            DebugDraw::g_Batch.get(),
            o,
            end - o,
            XMVectorSet(0, 1, 0, 1),
            false);
    }
}