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
    int CountNonSpaceChars(const wchar_t* s)
    {
        if (!s) return 0;

        int count = 0;
        for (const wchar_t* p = s; *p; ++p)
        {
            if (!iswspace(*p)) // 공백/개행/탭 제외
                ++count;
        }
        return count;
    }


    // 글자 수에 따른 말풍선 크기 단계 선택
    float SelectBubbleWidthByLen(int len)
    {
        if (len <= 6)   return 140.0f;    // "음..."
        if (len <= 10)  return 350.0f;    // "이정도 길이면 ?"
        if (len <= 20)  return 650.0f;     
        if (len <= 24)  return 1000.0f;    
        if (len <= 28)  return 1200.0f;    
        if (len <= 36)  return 1500.0f;
        if (len <= 44)  return 1700.0f;
        if (len <= 52)  return 1850.0f;

        return 2000.0f;                   // 상한 2000
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
    //    ShowDialogueText(L"저 곳을 지나갈 때 조심하지 않으면 엄청 시끄러운 소리가 날 것 같아.");   // 0.5배 늘려
    //if (Input::GetKeyDown(Keyboard::K))
    //    ShowDialogueText(L"뭔가 더 으스스해진 기분인데?");   // 딱좋음
    //if (Input::GetKeyDown(Keyboard::J))
    //    ShowDialogueText(L"여기서 음식을 만들 수 있겠어.망치지 않게 집중해야지!");    // 0.25배 늘리기
    //if (Input::GetKeyDown(Keyboard::H))
    //    ShowDialogueText(L"집안 어딘가에 이상한 기운이 생겼어... 확인해봐야겠어.");
    //if (Input::GetKeyDown(Keyboard::G))
    //    ShowDialogueText(L"이정도 길이면 ?");
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
    const int len = CountNonSpaceChars(s);
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

    const int len = CountNonSpaceChars(s);
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
    const int len = CountNonSpaceChars(s);
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