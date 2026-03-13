#include "FrozenManager.h"
#include "../../../Engine/Object/GameObject.h"
#include "../../../Engine/EngineSystem/SceneSystem.h"
#include "../../../Engine/Util/ComponentAutoRegister.h"
#include "../../../Engine/Util/JsonHelper.h"

REGISTER_COMPONENT(FrozenManager);

RTTR_REGISTRATION
{
    using namespace rttr;

    registration::class_<FrozenManager>("FrozenManager")
        .constructor<>()
            (rttr::policy::ctor::as_std_shared_ptr) 
        .property("maxTime", &FrozenManager::maxTime)
        .property("timer", &FrozenManager::timer)
        .property("len1", &FrozenManager::len1)
        .property("len2", &FrozenManager::len2)
        .property("len3", &FrozenManager::len3)
        .property("len4", &FrozenManager::len4)
        .property("len5", &FrozenManager::len5);
}

void FrozenManager::OnInitialize()
{
   effectsPath.resize(6);
   effectsPath[0] = "";
   effectsPath[1] = "..\\Assets\\Resource\\Effect\\Freezing\\Freezing_effect01.png";
   effectsPath[2] = "..\\Assets\\Resource\\Effect\\Freezing\\Freezing_effect02.png";
   effectsPath[3] = "..\\Assets\\Resource\\Effect\\Freezing\\Freezing_effect03.png";
   effectsPath[4] = "..\\Assets\\Resource\\Effect\\Freezing\\Freezing_effect04.png";
   effectsPath[5] = "..\\Assets\\Resource\\Effect\\Freezing\\Freezing_effect05.png";
}

void FrozenManager::OnStart()
{
    // 이미지 찾기
    frozenVignetteImage = GetOwner()->GetComponent<Image>();
    frozenVignetteImage->SetColor({1,1,1,0.8});

    // player 찾기
    pc = SceneUtil::GetObjectByName("Player")->GetComponent<PlayerController>();

    // 적 찾기
    auto enemies = SceneUtil::GetObjectsByName("Ghost_Adult");
    for (auto obj : enemies)
    {
        this->enemies.push_back(obj->GetComponent<AdultGhostController>());
    }
}

void FrozenManager::OnUpdate(float dt)
{
    if (isGameDirecting) return;
    if (enemies.empty()) return;
    if (!pc) return;
    if (!frozenVignetteImage) return;

    SetImageByDistance(dt);
}

nlohmann::json FrozenManager::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void FrozenManager::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

float FrozenManager::GetLengthFromNearestEnemy()
{
    if (enemies.empty()) return 0.0f;
    if (!pc) return 0.0f;

    Transform* target = nullptr;
    float minLength = 0x7fffffff;
    auto playerTransform = pc->GetOwner()->GetTransform();

    for (auto obj : enemies)
    {
        auto enemy = obj->GetOwner()->GetTransform();
        float dist = (enemy->GetWorldPosition() - playerTransform->GetWorldPosition()).Length();

        // 더 가까운 적 찾음 -> 갱신
        if (dist < minLength)
        {
            minLength = dist;
            target = enemy;
        }
    }

    return minLength;
}

void FrozenManager::SetImageByDistance(float dt)
{
    auto len = GetLengthFromNearestEnemy();

    if (len <= len1 && len > len2)
    {
        // 1단계
        nextfrozenLevel = 1;
    }
    else if (len <= len2 && len > len3)
    {
        // 2단계
        nextfrozenLevel = 2;
    }
    else if (len <= len3 && len > len4)
    {
        // 3단계
        nextfrozenLevel = 3;
    }
    else if (len <= len4 && len > len5)
    {
        // 4단계
        nextfrozenLevel = 4;
    }
    else if (len <= len5)
    {
        // 5단계
        nextfrozenLevel = 5;
    }
    else
    {
        // 0단계 - UI 비활성화
        nextfrozenLevel = 0;
    }

    // 단계 변화에 따른 이미지 변화
    if (nextfrozenLevel > frozenLevel)
    {
        frozenLevel = nextfrozenLevel;
        frozenVignetteImage->SetActive(true);
        frozenVignetteImage->ChangeData(effectsPath[frozenLevel]);
        timer = maxTime;
    }
    else if (nextfrozenLevel < frozenLevel)
    {
        timer -= dt;

        if (timer <= 0.0f)
        {
            // 감소
            frozenLevel--;
            if (frozenLevel == 0)
            {
                frozenVignetteImage->SetActive(false);
            }
            else
            {
                frozenVignetteImage->SetActive(true);
                frozenVignetteImage->ChangeData(effectsPath[frozenLevel]);
            }

            timer = maxTime; 
        }
    }
}