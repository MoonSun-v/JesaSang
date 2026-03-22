#include "AdultGhostWaypoint_A.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

REGISTER_COMPONENT(AdultGhostWaypoint_A)

RTTR_REGISTRATION
{
    rttr::registration::class_<AdultGhostWaypoint_A>("AdultGhostWaypoint_A")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

nlohmann::json AdultGhostWaypoint_A::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void AdultGhostWaypoint_A::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}


void AdultGhostWaypoint_A::OnInitialize()
{
    auto controller = GetOwner()->GetComponent<AdultGhostController>();
    if (!controller)
    {
        std::cout << "[AdultGhostWaypoint_A] AdultGhostController Missing" << std::endl;
        return;
    }

    controller->patrolPointCount = 11;

    controller->patrolPoints[0] = GridPos(8, -7);
    controller->patrolPoints[1] = GridPos(8, 0);
    controller->patrolPoints[2] = GridPos(3, 0);
    controller->patrolPoints[3] = GridPos(3, -4);
    controller->patrolPoints[4] = GridPos(-4, -4);
    controller->patrolPoints[5] = GridPos(-4, -18);
    controller->patrolPoints[6] = GridPos(-9, -18);
    controller->patrolPoints[7] = GridPos(4, -18);
    controller->patrolPoints[8] = GridPos(4, -11);
    controller->patrolPoints[9] = GridPos(6, -11);
    controller->patrolPoints[10] = GridPos(8, -10);
}
