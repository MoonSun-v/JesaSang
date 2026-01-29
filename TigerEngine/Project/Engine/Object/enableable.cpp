#include "Enableable.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<Enableable>("Enableable")
        .property("isActive", &Enableable::GetActiveSelf, &Enableable::SetActive);
}