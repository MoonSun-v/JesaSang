#include "Decal.h"
#include "../EngineSystem/DecalSystem.h"
#include "../Util/JsonHelper.h"

RTTR_REGISTRATION
{
    rttr::registration::class_<Decal>("Decal")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr);
}

void Decal::OnInitialize()
{
    DecalSystem::Instance().Register(this);
}

void Decal::OnDestory()
{
    DecalSystem::Instance().UnRegister(this);
}

nlohmann::json Decal::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void Decal::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}