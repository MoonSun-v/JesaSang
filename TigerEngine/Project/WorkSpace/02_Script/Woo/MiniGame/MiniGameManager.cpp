#include "MiniGameManager.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"
#include "EngineSystem/SceneSystem.h"
#include "System/InputSystem.h"
#include "Object/GameObject.h"
#include "Components/RectTransform.h"
#include "Components/UI/Image.h"

#include "../Item/Item.h"
#include "Game/IMinigame.h"
#include "Game/Game_Cutting.h"
#include "Game/Game_FireControl.h"
#include "Game/Game_Assembling.h"

#include "../Player/PlayerController.h"
#include "../../Ho/Sound/SoundManager.h"

REGISTER_COMPONENT(MiniGameManager)

RTTR_REGISTRATION
{
    rttr::registration::class_<MiniGameManager>("MiniGameManager")
    .constructor<>()
    (rttr::policy::ctor::as_std_shared_ptr);
}

void MiniGameManager::OnInitialize()
{
    // 중복 생성 방지
    if (s_instance != nullptr && s_instance != this)
    {
        assert(false && "Duplicate GameManager instance detected.");
        return;
    }

    s_instance = this;
}

void MiniGameManager::OnStart()
{
    miniGameParent = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("UI_MiniGame")->GetComponent<RectTransform>();
    success = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Image_Result_Success");
    fail = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Image_Result_Fail");
    miniGame1_Parent = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("UI_MiniGame1");
    miniGame2_Parent = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("UI_MiniGame2");
    miniGame3_Parent = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("UI_MiniGame3");

    if (!miniGameParent || !success || !fail ||
        !miniGame1_Parent || !miniGame2_Parent || !miniGame3_Parent)
    {
        cout << "[MiniGameManager] Missing ui objects...!!!" << endl;
    }

    // init
    popupHiddenPos = Vector3(960, 1500, 0);
    popupShownPos = Vector3(960, 550, 0);

    popupTargetPos = popupHiddenPos;
    miniGameParent->SetPos(popupHiddenPos);

    // result pause init
    isResultWaiting = false;
    resultWaitTimer = 0.0f;
    cachedResultSuccess = false;
    pendingEnd = false;
}

void MiniGameManager::OnUpdate(float delta)
{
    // popup move
    UpdatePopup(delta);

    // stop cheking
    StopChecking();

    // return (none game)
    if (!isPlaying || !currentMiniGame) return;

    // -------------------------
    // [Result Pause]
    // 성공/실패가 난 뒤, 잠깐(2초) 결과를 보여주고 팝업을 내리기만 한다.
    // 팝업이 완전히 내려간 뒤 EndMiniGame() 호출은 UpdatePopup()에서 처리.
    // -------------------------
    if (isResultWaiting)
    {
        resultWaitTimer += delta;
        if (resultWaitTimer >= resultWaitDuration)
        {
            isResultWaiting = false;
            resultWaitTimer = 0.0f;

            // 결과 저장(연출/반환용)
            cachedResultSuccess = currentMiniGame ? currentMiniGame->IsSuccess() : cachedResultSuccess;

            // 이제 팝업을 내리고, 완전히 내려간 뒤 EndMiniGame 호출하도록 플래그만 세팅
            pendingEnd = true;
            HidePopup();
        }
        return; // 대기 중엔 게임 업데이트 안 함(멈춘 것처럼 보임)
    }

    // game update
    currentMiniGame->UpdateGame(delta);
    currentMiniGame->UpdateAnimation(delta); // 애니메이션 실행

    // 게임이 끝나면 대기모드 진입
    if (currentMiniGame->IsFinished())
    {
        cachedResultSuccess = currentMiniGame->IsSuccess();
        isResultWaiting = true;
        resultWaitTimer = 0.0f;

        // 게임 결과 UI
        if(cachedResultSuccess)
            success->SetActive(true);
        else
            fail->SetActive(true);
    }
}

void MiniGameManager::OnDestory()
{
    if (s_instance == this) s_instance = nullptr;
    CleanupMinigame();
}

nlohmann::json MiniGameManager::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void MiniGameManager::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

std::unique_ptr<IMiniGame> MiniGameManager::CreateMinigameForIngredientId(const std::string& foodId)
{
    // 음식 재료에 따른 미니게임 선택
    if (foodId == "Ingredient_Apple" || foodId == "Ingredient_Pear")
    {
        miniGame1_Parent->SetActive(true);
        miniGame2_Parent->SetActive(false);
        miniGame3_Parent->SetActive(false);
        return std::make_unique<Game_Cutting>();
    }
    if (foodId == "Ingredient_Batter" || foodId == "Ingredient_Tofu")
    {
        miniGame1_Parent->SetActive(false);
        miniGame2_Parent->SetActive(true);
        miniGame3_Parent->SetActive(false);
        return std::make_unique<Game_FireControl>();
    }
    if (foodId == "Ingredient_Sanjeok" || foodId == "Ingredient_Donggeurangttaeng")
    {
        miniGame1_Parent->SetActive(false);
        miniGame2_Parent->SetActive(false);
        miniGame3_Parent->SetActive(true);
        return std::make_unique<Game_Assembling>();
    }

    return nullptr;
}

void MiniGameManager::CleanupMinigame()
{
    if (currentMiniGame)
    {
        currentMiniGame->EndGame();
        currentMiniGame.reset();
    }
    curIngredient.reset();
}

void MiniGameManager::StartMiniGame(std::unique_ptr<IItem> ingredient)
{
    if (!ingredient)
        return;

    if (ingredient->itemType != ItemType::Ingredient)
        return;

    miniGameParent->GetOwner()->SetActive(true);
    success->SetActive(false);
    fail->SetActive(false);

    // ---- result pause reset ----
    isResultWaiting = false;
    resultWaitTimer = 0.0f;
    cachedResultSuccess = false;
    pendingEnd = false;
    // ----------------------------

    // mini game create
    const std::string foodId = ingredient->itemId;
    auto newGame = CreateMinigameForIngredientId(foodId);
    if (!newGame) return;

    // ownership
    curIngredient = std::move(ingredient);
    currentMiniGame = std::move(newGame);

    // start game
    currentMiniGame->StartGame();
    isPlaying = true;

    // popup ui
    ShowPopup();

    // sound
    SoundManager::Instance()->PlaySFX(SFXType::Minigame_Interaction_Sound);
}

// 미니게임 종료시 (성공/실패+패널티)
void MiniGameManager::EndMiniGame(bool isSuccess)
{
    isPlaying = false;

    // end game
    if (currentMiniGame)
    {
        currentMiniGame->EndGame();
        currentMiniGame.reset();
    }

    // get playercontroller
    auto* pc = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Player")->GetComponent<PlayerController>();
    if (!pc)
    {
        cout << "[MiniGameManager] Missing Player Controller!" << endl;
        return;
    }

    // result return
    if (curIngredient)
        pc->ReceiveMiniGameResult(std::move(curIngredient), isSuccess);

    // player fsm change
    pc->ChangeState(PlayerState::Idle);
}

// 미니게임 강제 종료시 (재료만 반환)
void MiniGameManager::StopMiniGame()
{
    // 강제종료는 "대기 없이 즉시 종료" (원하는 동작)
    isPlaying = false;

    // result pause reset (혹시 성공/실패 대기중이었다면 끊어줌)
    isResultWaiting = false;
    resultWaitTimer = 0.0f;
    pendingEnd = false;

    // popup ui
    HidePopup();

    // end game
    if (currentMiniGame)
    {
        currentMiniGame->EndGame();
        currentMiniGame.reset();
    }

    // get playercontroller
    auto* pc = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Player")->GetComponent<PlayerController>();
    if (!pc)
    {
        cout << "[MiniGameManager] Missing Player Controller!" << endl;
        return;
    }

    // ingredient 돌려주기
    if (curIngredient)
        pc->ReceiveMiniGameItem(std::move(curIngredient));

    // player fsm change
    pc->ChangeState(PlayerState::Idle);
}

void MiniGameManager::StopChecking()
{
    if (Input::GetKeyDown(stop_key) && isPlaying)
    {
        StopMiniGame();
    }
}

void MiniGameManager::UpdatePopup(float delta)
{
    if (!miniGameParent) return;

    Vector3 cur = miniGameParent->GetPos();
    Vector3 next = Vector3::Lerp(cur, popupTargetPos, std::min(1.0f, popupSpeed * delta));
    miniGameParent->SetPos(next);

    // 도착 판정
    Vector3 diff = popupTargetPos - next;
    if (diff.Length() < 0.5f)
    {
        miniGameParent->SetPos(popupTargetPos);

        // pause controll
        // 팝업이 완전히 다 내려간 뒤, 게임 일시정지 가능
        if (isPopupHiding)
        {
            isPopupHiding = false;
            isPopupHidden = true;

            // -------------------------
            // [중요]
            // 성공/실패 후 2초 대기 -> HidePopup() 했고,
            // 팝업이 완전히 내려간 순간 여기서 EndMiniGame 호출
            // -------------------------
            if (pendingEnd)
            {
                pendingEnd = false;
                EndMiniGame(cachedResultSuccess);
            }
        }
    }
}

void MiniGameManager::ShowPopup()
{
    popupTargetPos = popupShownPos;
    isPopupHiding = false;
    isPopupHidden = false;
}

void MiniGameManager::HidePopup()
{
    popupTargetPos = popupHiddenPos;
    isPopupHiding = true;
}
