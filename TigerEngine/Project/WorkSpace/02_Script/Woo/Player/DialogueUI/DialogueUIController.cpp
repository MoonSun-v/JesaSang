#include "DialogueUIController.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"
#include "../Base/Datas/ReflectionMedtaDatas.hpp"

#include "EngineSystem/SceneSystem.h"
#include "System/InputSystem.h"
#include "System/TimeSystem.h"
#include "Object/GameObject.h"
#include "Components/RectTransform.h"
#include "Components/UI/Image.h"
#include "Components/UI/TextUI.h"

#include <directxtk/Keyboard.h>
#include <cwctype>
#include <string>


REGISTER_COMPONENT(DialogueUIController)

RTTR_REGISTRATION
{
    rttr::registration::class_<DialogueUIController>("DialogueUIController")
    .constructor<>()
    (rttr::policy::ctor::as_std_shared_ptr);
}

namespace
{
    float EstimateDialogueLength(const wchar_t* s)
    {
        if (!s) return 0.0f;

        float count = 0.0f;
        for (const wchar_t* p = s; *p; ++p)
        {
            if (*p == L'\n' || *p == L'\r' || *p == L'\t')
                continue;
            else if (*p == L' ')
                count += 0.5f;
            else
                count += 1.0f;
        }
        return count;
    }


    // 글자 수에 따른 말풍선 크기 단계 선택
    float SelectBubbleWidthByLen(float len)
    {
        if (len <= 4)   return 120.0f;
        if (len <= 6)   return 160.0f;
        if (len <= 8)   return 240.0f;
        if (len <= 10)  return 320.0f;
        if (len <= 12)  return 420.0f;
        if (len <= 14)  return 520.0f;
        if (len <= 16)  return 620.0f;
        if (len <= 18)  return 720.0f;
        if (len <= 20)  return 820.0f;
        if (len <= 22)  return 920.0f;
        if (len <= 24)  return 1020.0f;
        if (len <= 26)  return 1120.0f;
        if (len <= 28)  return 1220.0f;
        if (len <= 30)  return 1320.0f;
        if (len <= 33)  return 1450.0f;
        if (len <= 36)  return 1580.0f;
        if (len <= 39)  return 1700.0f;
        if (len <= 42)  return 1800.0f;
        if (len <= 46)  return 1900.0f;
        if (len <= 50)  return 2000.0f;
        if (len <= 54)  return 2100.0f;


        return 2200.0f;
    }
}


void DialogueUIController::OnStart()
{
    targetTr = GetOwner()->GetChildByName("DialoguePoint");

    auto curScene = SceneSystem::Instance().GetCurrentScene();

    auto parentGO = curScene->GetGameObjectByName("UI_PlayerDialogue");
    auto imgGO = curScene->GetGameObjectByName("Image_Dialogue");
    auto textGO = curScene->GetGameObjectByName("Text_Dialoaue"); // 오브젝트 이름 실제 값 확인 필요

    dialogueParent = parentGO ? parentGO->GetTransform() : nullptr;
    dialogueRect = imgGO ? imgGO->GetComponent<RectTransform>() : nullptr;
    text_dialogue = textGO ? textGO->GetComponent<TextUI>() : nullptr;
    
    if (!targetTr || !dialogueParent || !text_dialogue || !dialogueRect)
    {
        std::cout << "[DialogueUIController] Missing Components!" << std::endl;
    }

    // init
    DialogueOnOff(false);
}

void DialogueUIController::OnUpdate(float delta)
{
    if (!targetTr || !dialogueParent || !text_dialogue)
        return;

    // position trace
    dialogueParent->SetPosition(targetTr->GetWorldPosition() + offset);

    // 연출
    if (isDialogueOn && useAutoClose)
    {
        float unscaledDelta = GameTimer::Instance().UnscaledDeltaTime();
        dialogueTimer += unscaledDelta;
        if (dialogueTimer >= dialogueDuration)
        {
            dialogueTimer = 0.0f;
            isDialogueOn = false;
            DialogueOnOff(false);

            // 게임 재개
            GameTimer::Instance().SetTimeScale(1.0f);
        }
    }

    // --- test --- 
    // TODO :: Delete
    //if (Input::GetKeyDown(Keyboard::P))
    //{
    //    DialogueToggle();
    //}
    //
    //if (Input::GetKeyDown(Keyboard::L))
    //    ShowDialogueText(L"집안 어딘가에 이상한 기운이 생겼어... 확인해봐야겠어.");   
    //if (Input::GetKeyDown(Keyboard::K))
    //    ShowDialogueText(L"재료를 되찾았어. 어서 요리를 하러 가야겠어");  
    //if (Input::GetKeyDown(Keyboard::J))
    //    ShowDialogueText(L"완성된 음식을 무사히 되찾았어. 제사상으로 가져가자.");    
    //if (Input::GetKeyDown(Keyboard::H))
    //    ShowDialogueText(L"갑자기 왜 이렇게 춥지..? 뭔가 불길해..!!");   
    //if (Input::GetKeyDown(Keyboard::G))
    //    ShowDialogueText(L"일단 숨어서 상황을 지켜보자..!"); 
}

void DialogueUIController::OnDestory()
{

}

nlohmann::json DialogueUIController::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void DialogueUIController::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void DialogueUIController::DialogueOnOff(bool flag)
{
    if (!dialogueParent) return;

    // On -> 자동종료 트리거
    isDialogueOn = flag;
    if (flag) dialogueTimer = 0.0f;

    dialogueParent->GetOwner()->SetActive(flag);
}

void DialogueUIController::DialogueToggle()
{
    if (!dialogueParent) return;
    DialogueOnOff(!dialogueParent->GetOwner()->GetActiveSelf());
}

void DialogueUIController::ShowDialogueText(const wchar_t* s)
{
    if (!text_dialogue || !s) return;

    // text길이에 따른 image size 조절
    const float len = EstimateDialogueLength(s);
    const float w = SelectBubbleWidthByLen(len);

    if (dialogueRect)
    {
        Vector2 size = dialogueRect->GetSize();
        size.x = w;
        dialogueRect->SetSize(size);
    }

    // uapte text
    text_dialogue->SetText(std::wstring(s));

    useAutoClose = true; // 선민 | 02.11 

    // 자동 연출 시작
    DialogueOnOff(true);
}

// 고정 유지 버전 
void DialogueUIController::ShowDialogueHold(const wchar_t* s)
{
    if (!text_dialogue || !s) return;

    const float len = EstimateDialogueLength(s);
    const float w = SelectBubbleWidthByLen(len);

    if (dialogueRect)
    {
        Vector2 size = dialogueRect->GetSize();
        size.x = w;
        dialogueRect->SetSize(size);
    }

    text_dialogue->SetText(std::wstring(s));

    useAutoClose = false;   // 자동 종료 끔
    DialogueOnOff(true);
}


void DialogueUIController::ShowInteractionHintAndPause(const wchar_t* s)
{
    if (!text_dialogue || !s) return;

    // text길이에 따른 image size 조절
    const float len = EstimateDialogueLength(s);
    const float w = SelectBubbleWidthByLen(len);

    if (dialogueRect)
    {
        Vector2 size = dialogueRect->GetSize();
        size.x = w;
        dialogueRect->SetSize(size);
    }

    // uapte text
    text_dialogue->SetText(std::wstring(s));

    // 게임 정지
    GameTimer::Instance().SetTimeScale(0.0);

    // 자동 연출 시작
    DialogueOnOff(true);
}


// --------------------------------

void DialogueUIController::SetdialogueTarget(Transform* tr)
{
    targetTr = tr;
}