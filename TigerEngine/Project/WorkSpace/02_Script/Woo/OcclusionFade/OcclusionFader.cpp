#include "OcclusionFader.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

#include "Object/GameObject.h"
#include "Components/Camera.h"
#include "EngineSystem/SceneSystem.h"

#include "OcclusionFadeObject.h"

#include <algorithm>
#include <cmath>


REGISTER_COMPONENT(OcclusionFader)

RTTR_REGISTRATION
{
    rttr::registration::class_<OcclusionFader>("OcclusionFader")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

void OcclusionFader::OnStart()
{
    // get components
    transform = GetOwner()->GetComponent<Transform>();
    targetTr = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("OcclusionFadePoint")->GetComponent<Transform>();

    if (!transform || !targetTr)
    {
        cout << "[OcclusionFader] OnStart Missing Component" << endl;
        return;
    }

    // buffer resize
    hitBuffer.reserve(32);
}

void OcclusionFader::OnUpdate(float delta)
{
    if (!transform || !targetTr) return;

    currentHits.clear();
    hitBuffer.clear();

    // direction, distance 
    Vector3 origin = transform->GetWorldPosition(); 
    Vector3 target = targetTr->GetWorldPosition();
    Vector3 dir = target - origin;
    float dist = dir.Length();
    dir.Normalize();

    // raycast
    QueryTriggerInteraction triggerInteraction = QueryTriggerInteraction::Ignore;

    bool hit = PhysicsSystem::Instance().Raycast(
        origin,
        dir,
        dist,
        hitBuffer,
        CollisionLayer::Default,
        triggerInteraction,
        true
    );

    // current hit objects update
    if (hit)
    {
        for (auto& hit : hitBuffer)
        {
            OcclusionFadeObject* fadeObj = FindFadeObjectFromHit(hit);
            if (fadeObj)
                currentHits.insert(fadeObj);
        }
    }

    // FadeOut
    for (OcclusionFadeObject* obj : previousHits)
    {
        if (!obj) continue;
        if (currentHits.find(obj) == currentHits.end())
            obj->StartFadeOut();
    }

    // FadeIn
    for (OcclusionFadeObject* obj : currentHits)
    {
        if (!obj) continue;
        if (previousHits.find(obj) == previousHits.end())
            obj->StartFadeIn();
    }

    // previous hit objects update
    previousHits.swap(currentHits);
}

void OcclusionFader::OnDestory()
{
    previousHits.clear();
    currentHits.clear();
    hitBuffer.clear();
}

nlohmann::json OcclusionFader::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void OcclusionFader::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

OcclusionFadeObject* OcclusionFader::FindFadeObjectFromHit(const RaycastHit& hit)
{
    if (!hit.component) return nullptr;

    GameObject* hitObject = hit.component->GetOwner();
    if (!hitObject) return nullptr;

    // target 뒤에 있으면 return
    Vector3 origin = transform->GetWorldPosition();
    Vector3 targetPos = targetTr->GetWorldPosition();

    float targetDist = (targetPos - origin).Length();
    Vector3 hitPos(
        hit.point.x,
        hit.point.y,
        hit.point.z
    );
    float hitDist = (hitPos - origin).Length();

    if (hitDist > targetDist)
        return nullptr;

    // hit gameoject의 occlusion fade object 컴포넌트 반환
    if (OcclusionFadeObject* fadeComp = hitObject->GetComponent<OcclusionFadeObject>())
        return fadeComp;

    return nullptr;
}
