#include "GamePhaseManager.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"
#include "EngineSystem/SceneSystem.h"
#include "Object/GameObject.h"


REGISTER_COMPONENT(GamePhaseManager)

RTTR_REGISTRATION
{
    rttr::registration::class_<GamePhaseManager>("GamePhaseManager")
    .constructor<>()
    (rttr::policy::ctor::as_std_shared_ptr);
}

void GamePhaseManager::OnInitialize()
{
    // 중복 생성 방지
    if (s_instance != nullptr && s_instance != this)
    {
        assert(false && "Duplicate GameManager instance detected.");
        return;
    }

    s_instance = this;
}

void GamePhaseManager::OnStart()
{

}

void GamePhaseManager::OnUpdate(float dt)
{

}

void GamePhaseManager::OnDestory()
{
    if (s_instance == this) s_instance = nullptr;
}

nlohmann::json GamePhaseManager::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void GamePhaseManager::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}