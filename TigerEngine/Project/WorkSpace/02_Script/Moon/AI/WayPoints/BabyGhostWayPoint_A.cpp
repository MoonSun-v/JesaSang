#include "BabyGhostWayPoint_A.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

REGISTER_COMPONENT(BabyGhostWayPoint_A)

RTTR_REGISTRATION
{
    rttr::registration::class_<BabyGhostWayPoint_A>("BabyGhostWayPoint_A")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

nlohmann::json BabyGhostWayPoint_A::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void BabyGhostWayPoint_A::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}


void BabyGhostWayPoint_A::OnStart()
{
    auto controller = GetOwner()->GetComponent<BabyGhostController>();
    if (!controller)
    {
        std::cout << "[BabyGhostWayPoint_A] BabyGhostController Missing" << std::endl;
        return;
    }

    controller->patrolPointCount = 8;

    controller->patrolPoints[0] = { -13,-8,true };
    controller->patrolPoints[1] = { -13,-4,true };
    controller->patrolPoints[2] = { -9,-4,true };
    controller->patrolPoints[3] = { -9,-10,true };
    controller->patrolPoints[4] = { -12,-10,true };
    controller->patrolPoints[5] = { -12,-13,true };
    controller->patrolPoints[6] = { -14,-13,true };
    controller->patrolPoints[7] = { -14,-8,true };
}