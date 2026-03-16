#include "AdultGhostWaypoint_B.h"
#include "../AdultGhostController.h"

#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

REGISTER_COMPONENT(AdultGhostWaypoint_B)

RTTR_REGISTRATION
{
    rttr::registration::class_<AdultGhostWaypoint_B>("AdultGhostWaypoint_B")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

nlohmann::json AdultGhostWaypoint_B::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void AdultGhostWaypoint_B::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}



void AdultGhostWaypoint_B::OnStart()
{
    auto controller = GetOwner()->GetComponent<AdultGhostController>();
    if (!controller)
    {
        std::cout << "[AdultGhostWaypoint_B] AdultGhostController Missing" << std::endl;
        return;
    }

    controller->patrolPointCount = 4;

    controller->patrolPoints[0] = GridPos(-7, 9);
    controller->patrolPoints[1] = GridPos(-7, 4);
    controller->patrolPoints[2] = GridPos(-17, 4);
    controller->patrolPoints[3] = GridPos(-17, -8);
}
