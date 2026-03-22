#include "BabyGhostWayPoint_B.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

REGISTER_COMPONENT(BabyGhostWayPoint_B)

RTTR_REGISTRATION
{
    rttr::registration::class_<BabyGhostWayPoint_B>("BabyGhostWayPoint_B")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

nlohmann::json BabyGhostWayPoint_B::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void BabyGhostWayPoint_B::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}


void BabyGhostWayPoint_B::OnInitialize()
{
    auto controller = GetOwner()->GetComponent<BabyGhostController>();
    if (!controller)
    {
        std::cout << "[BabyGhostWayPoint_B] BabyGhostController Missing" << std::endl;
        return;
    }

    controller->patrolPointCount = 14;

    controller->patrolPoints[0] = { -3,-4,true };
    controller->patrolPoints[1] = { 4,-4,true };
    controller->patrolPoints[2] = { 4,-6,true };
    controller->patrolPoints[3] = { 3,-6,true };
    controller->patrolPoints[4] = { 3,-5,true };
    controller->patrolPoints[5] = { -4,-5,true };
    controller->patrolPoints[6] = { -4,-12,true };
    controller->patrolPoints[7] = { -4,-10,true };
    controller->patrolPoints[8] = { 0,-10,true };
    controller->patrolPoints[9] = { 0,-8,true };
    controller->patrolPoints[10] = { -4,-8,true };
    controller->patrolPoints[11] = { -4,-4,true };
    controller->patrolPoints[12] = { -6,-4,true };
}