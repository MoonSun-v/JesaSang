#include "BabyGhostWayPoint_E.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

REGISTER_COMPONENT(BabyGhostWayPoint_E)

RTTR_REGISTRATION
{
    rttr::registration::class_<BabyGhostWayPoint_E>("BabyGhostWayPoint_E")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

nlohmann::json BabyGhostWayPoint_E::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void BabyGhostWayPoint_E::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}


void BabyGhostWayPoint_E::OnStart()
{
    auto controller = GetOwner()->GetComponent<BabyGhostController>();
    if (!controller)
    {
        std::cout << "[BabyGhostWayPoint_E] BabyGhostController Missing" << std::endl;
        return;
    }

    controller->patrolPointCount = 8;

    controller->patrolPoints[0] = { -5,14,true };
    controller->patrolPoints[1] = { -18,14,true };
    controller->patrolPoints[2] = { -18,11,true };
    controller->patrolPoints[3] = { -17,11,true };
    controller->patrolPoints[4] = { -17,14,true };
    controller->patrolPoints[5] = { -7,14,true };
    controller->patrolPoints[6] = { -7,12,true };
    controller->patrolPoints[7] = { -5,12,true };
}