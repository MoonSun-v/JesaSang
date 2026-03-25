#include "BabyGhostWayPoint_D.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

REGISTER_COMPONENT(BabyGhostWayPoint_D)

RTTR_REGISTRATION
{
    rttr::registration::class_<BabyGhostWayPoint_D>("BabyGhostWayPoint_D")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

nlohmann::json BabyGhostWayPoint_D::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void BabyGhostWayPoint_D::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}


void BabyGhostWayPoint_D::OnInitialize()
{
    auto controller = GetOwner()->GetComponent<BabyGhostController>();
    if (!controller)
    {
        std::cout << "[BabyGhostWayPoint_D] BabyGhostController Missing" << std::endl;
        return;
    }

    controller->patrolPointCount = 14;

    controller->patrolPoints[0] = { 14,-3,true };
    controller->patrolPoints[1] = { 14,5,true };
    controller->patrolPoints[2] = { 10,5,true };
    controller->patrolPoints[3] = { 10,6,true };
    controller->patrolPoints[4] = { 7,6,true };
    controller->patrolPoints[5] = { 7,9,true };
    controller->patrolPoints[6] = { 10,9,true };
    controller->patrolPoints[7] = { 10,5,true };
    controller->patrolPoints[8] = { 17,5,true };
    controller->patrolPoints[9] = { 17,-6,true };
    controller->patrolPoints[10] = { 6,-6,true };
    controller->patrolPoints[11] = { 6,-7,true };
    controller->patrolPoints[12] = { 17,-7,true };
}