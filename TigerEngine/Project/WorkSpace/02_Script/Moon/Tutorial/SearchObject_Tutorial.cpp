#include "SearchObject_Tutorial.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

#include "Components/UI/Image.h"
#include "System/InputSystem.h"

REGISTER_COMPONENT(SearchObject_Tutorial)

RTTR_REGISTRATION
{
    rttr::registration::class_<SearchObject_Tutorial>("SearchObject_Tutorial")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

nlohmann::json SearchObject_Tutorial::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void SearchObject_Tutorial::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}


void SearchObject_Tutorial::OnStart()
{
    auto t1 = GetOwner()->GetChildByName("Image_SensorOn");
    auto t2 = GetOwner()->GetChildByName("Image_InteractionOn");
    auto t3 = GetOwner()->GetChildByName("Image_InteractionGauge");

    image_sensorOn = t1->GetOwner()->GetComponent<Image>();
    image_interactionOn = t2->GetOwner()->GetComponent<Image>();
    image_interactionGauge = t3->GetOwner()->GetComponent<Image>();

    ShowUI(false);
}

void SearchObject_Tutorial::ShowUI(bool on)
{
    image_sensorOn->SetActive(on);
    image_interactionOn->SetActive(on);
    image_interactionGauge->SetActive(on);
}

bool SearchObject_Tutorial::UpdateInteraction(float dt)
{
    if (isCompleted) return true;

    if (Input::GetKey(Keyboard::F))
    {
        gauge += dt / holdTime;
        image_interactionGauge->SetFillAmount(gauge);

        if (gauge >= 1.0f)
        {
            isCompleted = true;
            ShowUI(false);
            return true;
        }
    }
    else
    {
        gauge = 0.0f;
        image_interactionGauge->SetFillAmount(0.0f);
    }

    return false;
}
