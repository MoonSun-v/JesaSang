#include "BabyGhostWayPoint_C.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

REGISTER_COMPONENT(BabyGhostWayPoint_C)

RTTR_REGISTRATION
{
    rttr::registration::class_<BabyGhostWayPoint_C>("BabyGhostWayPoint_C")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

nlohmann::json BabyGhostWayPoint_C::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void BabyGhostWayPoint_C::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}


void BabyGhostWayPoint_C::OnInitialize()
{
    auto controller = GetOwner()->GetComponent<BabyGhostController>();
    if (!controller)
    {
        std::cout << "[BabyGhostWayPoint_C] BabyGhostController Missing" << std::endl;
        return;
    }

    controller->patrolPointCount = 11;

    controller->patrolPoints[0] = { -3,1,true };
    controller->patrolPoints[1] = { -3,4,true };
    controller->patrolPoints[2] = { 0,4,true };
    controller->patrolPoints[3] = { 1,0,true };
    controller->patrolPoints[4] = { 3,4,true };
    controller->patrolPoints[5] = { 4,0,true };
    controller->patrolPoints[6] = { 8,0,true };
    controller->patrolPoints[7] = { 8,-2,true };
    controller->patrolPoints[8] = { 7,-2,true };
    controller->patrolPoints[9] = { 7,0,true };
    controller->patrolPoints[10] = { 0,0,true };
}