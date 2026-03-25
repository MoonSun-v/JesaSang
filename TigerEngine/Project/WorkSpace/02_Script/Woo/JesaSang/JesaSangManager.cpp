#include "JesaSangManager.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"
#include "EngineSystem/SceneSystem.h"
#include "Object/GameObject.h"
#include "Components/UI/Image.h"

#include "../Item/Item.h"
#include "../Manager/GameManager.h"
#include "../Manager/QuestManager.h"
#include "../Manager/GamePhaseManager.h"


REGISTER_COMPONENT(JesaSangManager)

RTTR_REGISTRATION
{
    rttr::registration::class_<JesaSangManager>("JesaSangManager")
    .constructor<>()
    (rttr::policy::ctor::as_std_shared_ptr);
}

void JesaSangManager::OnInitialize()
{
    // 중복 생성 방지
    if (s_instance != nullptr && s_instance != this)
    {
        assert(false && "Duplicate GameManager instance detected.");
        return;
    }

    s_instance = this;
}

void JesaSangManager::OnStart()
{
    // gameobject find
    const auto& sceneSystem = SceneSystem::Instance().GetCurrentScene();
    apple = sceneSystem->GetGameObjectByName("Apple");
    pear = sceneSystem->GetGameObjectByName("Pear");
    batter = sceneSystem->GetGameObjectByName("Batter");
    tofu = sceneSystem->GetGameObjectByName("Tofu");
    sanjeok = sceneSystem->GetGameObjectByName("Sanjeok");
    dong = sceneSystem->GetGameObjectByName("Donggeurangttaeng");

    if (!apple || !pear || !batter || !sanjeok || !dong)
        std::cout << "[JesaSangManager] Missing GameObject!" << endl;

    image_sensorOn = sceneSystem->GetGameObjectByName("Image_SensorOn_Jesasang")->GetComponent<Image>();
    image_interactionOn = sceneSystem->GetGameObjectByName("Image_InteractionOn_Jesasang")->GetComponent<Image>();
    image_interactionGauge = sceneSystem->GetGameObjectByName("Image_InteractionGauge_Jesasang")->GetComponent<Image>();

    if (!image_sensorOn || !image_interactionOn || !image_interactionGauge)
    {
        cout << "[SearchObject] Missing ui!" << endl;
        return;
    }
}

void JesaSangManager::OnDestory()
{
    if (s_instance == this) s_instance = nullptr;
}


nlohmann::json JesaSangManager::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void JesaSangManager::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

bool JesaSangManager::HasAllFood()
{
    return hasApple && hasPear && hasBatter && hasTofu && hasSanjeok && hasDong;
}

/*
    [Food Item ID]
    Apple
    Pear
    Batter
    Tofu
    Sanjeok
    Donggeurangttaeng
*/
void JesaSangManager::ReceiveFood(std::unique_ptr<IItem> food)
{
    string foodID = food->itemId;
    if (foodID.empty())
    {
        std::cout << "[JesaSangManager] Food ID NULL!" << endl;
        return;
    }

    // 음식 올리기
    if (foodID == "Apple" && apple)
    {
        hasApple = true;
        apple->SetActive(true);
    }
    else if (foodID == "Pear" && pear)
    {
        hasPear = true;
        pear->SetActive(true);
    }
    else if (foodID == "Batter" && batter)
    {
        hasBatter = true;
        batter->SetActive(true);
    }
    else if (foodID == "Tofu" && tofu)
    {
        hasTofu = true;
        tofu->SetActive(true);
    }
    else if (foodID == "Sanjeok" && sanjeok)
    {
        hasSanjeok = true;
        sanjeok->SetActive(true);
    }
    else if (foodID == "Donggeurangttaeng" && dong)
    {
        hasDong = true;
        dong->SetActive(true);
    }

    hasFoodCount++;

    // 전달받은 Food Item은 할일 끝났으니 쏘멸
    std::cout << "[JesaSangManager] Put Food : " << food->itemId << endl;
    food.reset();

    // UI clear
    UISensorOnOff(false);
    UIInteractionOnOff(false);
    image_interactionGauge->SetFillAmount(0.0);

    // 퀘스트 4 : [완성] 제사상 완성 : 제사상에 올라온 음식 카운팅 (1/6)
    QuestManager::Instance()->StepComplete(4);

    // 4개의 음식이 모였을때
    if(hasFoodCount == 4)
    {
        // TODO :: GamePhaseManager 연출 호출
    }

    // 제사상에 음식이 다 올라오면 성공
    if(HasAllFood())
        GameManager::Instance()->GameSuccess();
}

void JesaSangManager::UISensorOnOff(bool flag)
{
    if (!image_sensorOn) return;
    image_sensorOn->SetActive(flag);
}

void JesaSangManager::UIInteractionOnOff(bool flag)
{
    if (!image_interactionOn) return;
    image_interactionOn->SetActive(flag);
    image_interactionGauge->SetActive(flag);
}

void JesaSangManager::UIGaugeUpate(float progress)
{
    if (!image_interactionGauge) return;
    image_interactionGauge->SetFillAmount(progress);
}