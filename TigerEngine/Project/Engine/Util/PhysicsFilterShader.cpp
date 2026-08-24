#include "PhysicsFilterShader.h"

PxFilterFlags PhysicsFilterShader(
    PxFilterObjectAttributes attr0, PxFilterData data0,
    PxFilterObjectAttributes attr1, PxFilterData data1,
    PxPairFlags& pairFlags,
    const void*, PxU32)
{
    const bool isTriggerPair =
        PxFilterObjectIsTrigger(attr0) ||
        PxFilterObjectIsTrigger(attr1);

    // 1) Layer mask 검사
    if (!((data0.word0 & data1.word1) && (data1.word0 & data0.word1)))
        return PxFilterFlag::eSUPPRESS;

    // 2) Trigger ↔ Anything
    if (isTriggerPair)
    {
        pairFlags =
            PxPairFlag::eTRIGGER_DEFAULT |   // 기본 Trigger 세팅
            PxPairFlag::eNOTIFY_TOUCH_FOUND |
            PxPairFlag::eNOTIFY_TOUCH_LOST;

        return PxFilterFlag::eDEFAULT;
    }

    // 3) Simulation ↔ Simulation
    pairFlags =
        PxPairFlag::eCONTACT_DEFAULT |
        PxPairFlag::eNOTIFY_TOUCH_FOUND |
        PxPairFlag::eNOTIFY_TOUCH_PERSISTS |
        PxPairFlag::eNOTIFY_TOUCH_LOST;

    return PxFilterFlag::eDEFAULT;
}
