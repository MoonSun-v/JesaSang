// ===============================
// AltarManager.cpp
// ===============================
#include "AltarManager.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"
#include "EngineSystem/SceneSystem.h"
#include "EngineSystem/CameraSystem.h"
#include "Components/Camera.h"
#include "Object/GameObject.h"
#include "Components/UI/Image.h"
#include "Manager/WorldManager.h"
#include "System/TimeSystem.h"
#include "RenderPass/ParticleSource/Effect.h"
#include "../Player/DialogueUI/DialogueUIController.h"

#include "../Item/Item.h"
#include "../../Ho/Sound/SoundManager.h"
#include "../../Ron/MiniMapTest/MiniMapManager.h"

REGISTER_COMPONENT(AltarManager)

RTTR_REGISTRATION
{
    rttr::registration::class_<AltarManager>("AltarManager")
    .constructor<>()
    (rttr::policy::ctor::as_std_shared_ptr);
}

void AltarManager::OnInitialize()
{
    // 중복 생성 방지
    if (s_instance != nullptr && s_instance != this)
    {
        assert(false && "Duplicate GameManager instance detected.");
        return;
    }

    s_instance = this;
}

void AltarManager::OnStart()
{
    const auto& sceneSystem = SceneSystem::Instance().GetCurrentScene();

    // direct cam find
    GameObject* directCamObj = sceneSystem->GetGameObjectByName("AltarDirectCamera");
    if (!directCamObj)
    {
        cout << "[AltarManager] Missing AltarDirectCamera!" << endl;
    }
    else
    {
        altarDirectCam = directCamObj->GetComponent<Transform>();
        if (!altarDirectCam)
            cout << "[AltarManager] Missing Transform on AltarDirectCamera!" << endl;
        else
            altarDirectCam->GetOwner()->SetName(camName);
    }

    // direct effect find
    GameObject* directEffectbj = sceneSystem->GetGameObjectByName("AltarDirectEffect_Item");
    if (!directEffectbj)
    {
        cout << "[AltarManager] Missing fireEffect Object!" << endl;
    }
    else
    {
        fireEffect = directEffectbj->GetComponent<Effect>();
        if (!fireEffect)
            cout << "[AltarManager] Missing fireEffect!" << endl;
    }

    // gameobject find
    altar = sceneSystem->GetGameObjectByName("Altar");
    altarOffWall = sceneSystem->GetGameObjectByName("AltarOffWall");
    altarOnWall = sceneSystem->GetGameObjectByName("AltarOnWall");

    ingre_apple = sceneSystem->GetGameObjectByName("Alta_Ingre_Apple");
    ingre_pear = sceneSystem->GetGameObjectByName("Alta_Ingre_Pear");
    ingre_batter = sceneSystem->GetGameObjectByName("Alta_Ingre_Batter");
    ingre_tofu = sceneSystem->GetGameObjectByName("Alta_Ingre_Tofu");
    ingre_sanjeok = sceneSystem->GetGameObjectByName("Alta_Ingre_Sanjeok");
    ingre_dong = sceneSystem->GetGameObjectByName("Alta_Ingre_Donggeurangttaeng");

    food_apple = sceneSystem->GetGameObjectByName("Alta_Apple");
    food_pear = sceneSystem->GetGameObjectByName("Alta_Pear");
    food_batter = sceneSystem->GetGameObjectByName("Alta_Batter");
    food_tofu = sceneSystem->GetGameObjectByName("Alta_Tofu");
    food_sanjeok = sceneSystem->GetGameObjectByName("Alta_Sanjeok");
    food_dong = sceneSystem->GetGameObjectByName("Alta_Donggeurangttaeng");

    if (!altar || !altarOffWall || !altarOnWall)
    {
        cout << "[AltarManager] Missing Altar GameObject!" << endl;
        return;
    }

    if (!ingre_apple || !ingre_pear || !ingre_batter || !ingre_tofu || !ingre_sanjeok || !ingre_dong ||
        !food_apple || !food_pear || !food_batter || !food_tofu || !food_sanjeok || !food_dong)
    {
        cout << "[AltarManager] Missing GameObject!" << endl;
        return;
    }

    image_sensorOn = sceneSystem->GetGameObjectByName("Image_SensorOn_Altar")->GetComponent<Image>();
    image_interactionOn = sceneSystem->GetGameObjectByName("Image_InteractionOn_Altar")->GetComponent<Image>();
    image_interactionGauge = sceneSystem->GetGameObjectByName("Image_InteractionGauge_Altar")->GetComponent<Image>();

    if (!image_sensorOn || !image_interactionOn || !image_interactionGauge)
    {
        cout << "[AltarManager] Missing ui!" << endl;
        return;
    }

    // active init
    altar->SetActive(false);

    SetAllVisualOff();
    image_sensorOn->SetActive(false);
    image_interactionOn->SetActive(false);
    image_interactionGauge->SetActive(false);
}

void AltarManager::OnUpdate(float delta)
{
    if (!isDirecting) return;

    UpdateDirectSequence();
}

void AltarManager::OnDestory()
{
    if (s_instance == this) s_instance = nullptr;

    itemQueue.clear();
}

nlohmann::json AltarManager::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void AltarManager::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}


void AltarManager::FirstReceiveDirect(std::string itemId)
{
    // 제단 활성화 연출
    /*
    - 페이드아웃 → 페이드인 (이때 카메라가 제단을 정면에서 바라보는 view)
    - 카메라 줌인 → 제단에 푸른 불꽃 이펙트를 재생하며 음식 비주얼 on
    - 페이드아웃 → 페이드인(이때 원시점 복귀)
    */

    // 참고. 이 값만 바꾸면 알아서 포스트 프로세싱 데이터로 들어간다.
    // 이걸로 페이드(비네트) 연출 가능!
    auto& renderDesc = WorldManager::Instance().postProcessData;
    (void)renderDesc; // 아직 이펙트/페이드 로직은 미구현(나중에 추가)

    BeginDirectSequence(itemId);
}


/*
    [Item ID]

    Ingredient_Apple
    Ingredient_Pear
    Ingredient_Batter
    Ingredient_Tofu
    Ingredient_Sanjeok
    Ingredient_Donggeurangttaeng

    Apple
    Pear
    Batter
    Tofu
    Sanjeok
    Donggeurangttaeng
*/

void AltarManager::VisualItem(std::string itemID, bool isActive)
{
    if (itemID.empty()) return;

    // Ingredient
    if (itemID == "Ingredient_Apple" && ingre_apple) ingre_apple->SetActive(isActive);
    else if (itemID == "Ingredient_Pear" && ingre_pear) ingre_pear->SetActive(isActive);
    else if (itemID == "Ingredient_Batter" && ingre_batter) ingre_batter->SetActive(isActive);
    else if (itemID == "Ingredient_Tofu" && ingre_tofu) ingre_tofu->SetActive(isActive);
    else if (itemID == "Ingredient_Sanjeok" && ingre_sanjeok) ingre_sanjeok->SetActive(isActive);
    else if (itemID == "Ingredient_Donggeurangttaeng" && ingre_dong) ingre_dong->SetActive(isActive);

    // Food
    else if (itemID == "Apple" && food_apple) food_apple->SetActive(isActive);
    else if (itemID == "Pear" && food_pear) food_pear->SetActive(isActive);
    else if (itemID == "Batter" && food_batter) food_batter->SetActive(isActive);
    else if (itemID == "Tofu" && food_tofu) food_tofu->SetActive(isActive);
    else if (itemID == "Sanjeok" && food_sanjeok) food_sanjeok->SetActive(isActive);
    else if (itemID == "Donggeurangttaeng" && food_dong) food_dong->SetActive(isActive);
    else
        cout << "[AltarManager] Unknown itemID: " << itemID << endl;
}

void AltarManager::ReceiveItem(std::unique_ptr<IItem> item)
{
    if (!item) {
        cout << "[AltarManager] item null !" << endl;
        return;
    }

    if (isDirecting)
    {
        itemQueue.push_back(std::move(item));
        return;
    }

    // 최초 제단 활성화
    if (!isFirstReceiveItem)
    {
        isFirstReceiveItem = true;
        FirstReceiveDirect(item->itemId);

        // 아이템이 제단에 올라감(큐 적재)
        itemQueue.push_back(std::move(item));
        return;     // 비주얼은 연출쪽에서 알맞은 타이밍에 on
    }

    // 비주얼 on
    VisualItem(item->itemId, true);

    // 아이템이 제단에 올라감
    itemQueue.push_back(std::move(item));
}

std::unique_ptr<IItem> AltarManager::GetItem()
{
    if (isDirecting)
    {
        cout << "[AltarManager] directing... can't get item now." << endl;
        return nullptr;
    }

    // FIFO 회수
    if (!itemQueue.empty())
    {
        std::unique_ptr<IItem> out = std::move(itemQueue.front());
        itemQueue.pop_front();

        VisualItem(out->itemId, false);

        // UI clear
        UISensorOnOff(false);
        UIInteractionOnOff(false);
        image_interactionGauge->SetFillAmount(0.0);

        return out;
    }

    cout << "[AltarManager] no item !" << endl;
    return nullptr;
}

void AltarManager::UISensorOnOff(bool flag)
{
    if (!image_sensorOn) return;
    if (flag && !isFirstReceiveItem) return;
    if (flag && isDirecting) return;
    image_sensorOn->SetActive(flag);
}

void AltarManager::UIInteractionOnOff(bool flag)
{
    if (!image_interactionOn) return;
    if (flag && !isFirstReceiveItem) return;
    if (flag && isDirecting) return;
    image_interactionOn->SetActive(flag);
    image_interactionGauge->SetActive(flag);
}

void AltarManager::UIGaugeUpate(float progress)
{
    if (!image_interactionGauge) return;
    if (!isFirstReceiveItem) return;
    if (isDirecting) return;
    image_interactionGauge->SetFillAmount(progress);
}

// -------------------------------
// internal helpers
// -------------------------------
void AltarManager::SetAllVisualOff()
{
    if (ingre_apple) ingre_apple->SetActive(false);
    if (ingre_pear) ingre_pear->SetActive(false);
    if (ingre_batter) ingre_batter->SetActive(false);
    if (ingre_tofu) ingre_tofu->SetActive(false);
    if (ingre_sanjeok) ingre_sanjeok->SetActive(false);
    if (ingre_dong) ingre_dong->SetActive(false);

    if (food_apple) food_apple->SetActive(false);
    if (food_pear) food_pear->SetActive(false);
    if (food_batter) food_batter->SetActive(false);
    if (food_tofu) food_tofu->SetActive(false);
    if (food_sanjeok) food_sanjeok->SetActive(false);
    if (food_dong) food_dong->SetActive(false);

    UISensorOnOff(false);
    UIInteractionOnOff(false);
}

void AltarManager::BeginDirectSequence(std::string itemId)
{
    if (!altar)
    {
        cout << "[AltarManager] altar null! can't direct." << endl;
        return;
    }

    directingItemId = itemId;

    // 시작 상태 정리
    SetAllVisualOff();
    altar->SetActive(false);
    UISensorOnOff(false);
    UIInteractionOnOff(false);

    // direct cam 위치 캐싱
    hasDirectCamPosCached = false;
    if (altarDirectCam)
    {
        directCamStartPos = altarDirectCam->GetWorldPosition();
        directCamTargetPos = directCamStartPos;
        directCamTargetPos.z += 50.0f;
        hasDirectCamPosCached = true;
    }

    BackupPostProcess();

    // 게임 시간 정지
    GameTimer::Instance().SetTimeScale(0.0);

    // 연출 진입
    isDirecting = true;
    directPhase = DirectPhase::FadeOut_1;
    phaseTimer = 0.0f;

    StartVignetteFade(1.0f, 0.0f, fadeOutTime_1);
}

void AltarManager::UpdateDirectSequence()
{
    // 연출용 타이머
    auto udt = GameTimer::Instance().UnscaledDeltaTime();

    auto& renderDesc = WorldManager::Instance().postProcessData;
    (void)renderDesc;

    phaseTimer += udt;

    switch (directPhase)
    {
    case DirectPhase::FadeOut_1:
    {
        // 1. FadeOut
        UpdateVignetteFade();
        
        if (phaseTimer >= fadeOutTime_1)
        {
            phaseTimer = 0.0f;
            directPhase = DirectPhase::SwitchToDirectCam;
        }
    }
    break;

    case DirectPhase::SwitchToDirectCam:
    {
        // 2. 카메라 전환 (Direct Camera)
        CameraSystem::Instance().SetCurrCameraByName(camName);

        // 3. 최초 제단 활성화
        altar->SetActive(true);
        SetAllVisualOff();

        phaseTimer = 0.0f;
        directPhase = DirectPhase::FadeIn_1_And_ZoomIn;

        StartVignetteFade(0.0f, 1.0f, fadeInTime_1);
    }
    break;

    case DirectPhase::FadeIn_1_And_ZoomIn:
    {
        // 4. FadeIn + Cam position update
        UpdateVignetteFade();

        if (hasDirectCamPosCached && altarDirectCam)
        {
            const float t = (zoomInTime <= 0.0f) ? 1.0f : std::min(phaseTimer / zoomInTime, 1.0f);
            Vector3 pos = directCamStartPos * (1.0f - t) + directCamTargetPos * t;
            altarDirectCam->SetPosition(pos);
        }

        if (phaseTimer >= std::max(fadeInTime_1, zoomInTime))
        {
            phaseTimer = 0.0f;
            directPhase = DirectPhase::VisualOn;
        }
    }
    break;

    case DirectPhase::VisualOn:
    {
        // 5. Effect Play + Visual On
        VisualItem(directingItemId, true);

        if (!startedFireFx)
        {
            Vector3 localPos = GetPositionItem(directingItemId);
            localPos.y += 15;
            localPos.z -= 10;
            fireEffect->GetOwner()->GetTransform()->SetPosition(localPos);
            if (fireEffect) fireEffect->Play();
            startedFireFx = true;

            // sound
            SoundManager::Instance()->PlaySFX(SFXType::Altar_Fire_Sound);
        }

        if (visualHoldTime <= 0.0f || phaseTimer >= visualHoldTime)
        {
            phaseTimer = 0.0f;
            directPhase = DirectPhase::FadeOut_2;

            StartVignetteFade(1.0f, 0.0f, fadeOutTime_2);
        }
    }
    break;

    case DirectPhase::FadeOut_2:
    {
        // 6. FadeOut
        UpdateVignetteFade();

        if (phaseTimer >= fadeOutTime_2)
        {
            phaseTimer = 0.0f;
            directPhase = DirectPhase::SwitchToMainCam;
        }
    }
    break;

    case DirectPhase::SwitchToMainCam:
    {
        // 7. 카메라 전환 (Main Camera)
        CameraSystem::Instance().SetCurrCameraByName("MainCamera");

        phaseTimer = 0.0f;
        directPhase = DirectPhase::FadeIn_2;

        // 공간 오픈
        altarOffWall->SetActive(false);
        altarOnWall->SetActive(true);

        // 미니맵에 제단영역 활성화
        auto* ob = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("UI_MiniMap_Controller");
        if(ob) ob->GetComponent<MiniMapManager>()->TriggerShrineEvent(true);

        // sound
        SoundManager::Instance()->PlaySFX(SFXType::Altar_Mapchange_Sound);

        StartVignetteFade(0.0f, 1.0f, fadeInTime_2);
    }
    break;

    case DirectPhase::FadeIn_2:
    {
        // 8. FadeIn
        UpdateVignetteFade();

        if (phaseTimer >= fadeInTime_2)
        {
            phaseTimer = 0.0f;
            directPhase = DirectPhase::Done;
        }
    }
    break;

    case DirectPhase::Done:
    {
        // 연출 종료
        isDirecting = false;
        directPhase = DirectPhase::None;
        phaseTimer = 0.0f;

        // post data 복구
        RestorePostProcess();

        // 게임 시간 재개
        GameTimer::Instance().SetTimeScale(1.0);

        // Player 다이얼로그
        auto go = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Player");
        go->GetComponent<DialogueUIController>()->ShowDialogueText(L"집안 어딘가에 이상한 기운이 생겼어... 확인해봐야겠어.");

    }
    break;

    default:
        break;
    }
}

Vector3 AltarManager::GetPositionItem(std::string id)
{
    if (id.empty()) return { 0,0,0 };
    
    // Ingredient
    if (id == "Ingredient_Apple" && ingre_apple != nullptr) return ingre_apple->GetTransform()->GetLocalPosition();
    else if (id == "Ingredient_Pear" && ingre_pear != nullptr)  return ingre_pear->GetTransform()->GetLocalPosition();
    else if (id == "Ingredient_Batter" && ingre_batter != nullptr) return ingre_batter->GetTransform()->GetLocalPosition();
    else if (id == "Ingredient_Tofu" && ingre_tofu != nullptr)  return ingre_tofu->GetTransform()->GetLocalPosition();
    else if (id == "Ingredient_Sanjeok" && ingre_sanjeok != nullptr) return ingre_sanjeok->GetTransform()->GetLocalPosition();
    else if (id == "Ingredient_Donggeurangttaeng" && ingre_dong != nullptr)  return ingre_dong->GetTransform()->GetLocalPosition();
    
    // Food
    else if (id == "Apple" && food_apple != nullptr)  return food_apple->GetTransform()->GetLocalPosition();
    else if (id == "Pear" && food_pear != nullptr)  return food_pear->GetTransform()->GetLocalPosition();
    else if (id == "Batter" && food_batter != nullptr) return food_batter->GetTransform()->GetLocalPosition();
    else if (id == "Tofu" && food_tofu != nullptr) return food_tofu->GetTransform()->GetLocalPosition();
    else if (id == "Sanjeok" && food_sanjeok != nullptr)  return food_sanjeok->GetTransform()->GetLocalPosition();
    else if (id == "Donggeurangttaeng" && food_dong != nullptr)  return food_dong->GetTransform()->GetLocalPosition();
    
    return { 0,0,0 };
}

float AltarManager::Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

float AltarManager::EaseInOut(float t)
{
    // smoothstep
    t = Clamp01(t);
    return t * t * (3.0f - 2.0f * t);
}

void AltarManager::BackupPostProcess()
{
    auto& pp = WorldManager::Instance().postProcessData;

    vignetteBackup.valid = true;
    vignetteBackup.useVignette = pp.useVignette;
    vignetteBackup.intensity = pp.vignette_intensity;
    vignetteBackup.smoothness = pp.vignette_smoothness;
    vignetteBackup.center = pp.vignetteCenter;
    vignetteBackup.color = pp.vignetteColor;
}

void AltarManager::RestorePostProcess()
{
    if (!vignetteBackup.valid) return;

    auto& pp = WorldManager::Instance().postProcessData;

    pp.useVignette = vignetteBackup.useVignette;
    pp.vignette_intensity = vignetteBackup.intensity;
    pp.vignette_smoothness = vignetteBackup.smoothness;
    pp.vignetteCenter = vignetteBackup.center;
    pp.vignetteColor = vignetteBackup.color;

    vignetteBackup.valid = false;

    // 페이드 상태도 리셋
    vignetteFrom = vignetteTo = 0.0f;
    vignetteDuration = 0.0f;
}

void AltarManager::StartVignetteFade(float from, float to, float duration)
{
    auto& pp = WorldManager::Instance().postProcessData;

    pp.useVignette = true;
    pp.vignette_intensity = 1.0f;;
    pp.vignetteCenter = { 0.5f, 0.5f };
    pp.vignetteColor = { 0,0,0 };

    vignetteFrom = Clamp01(from);
    vignetteTo = Clamp01(to);
    vignetteDuration = (duration <= 0.0f) ? 0.0001f : duration;

    pp.vignette_smoothness = vignetteFrom;
}

void AltarManager::UpdateVignetteFade()
{
    if (vignetteDuration <= 0.0f) return;

    const float t = Clamp01(phaseTimer / vignetteDuration);
    const float e = EaseInOut(t);

    auto& pp = WorldManager::Instance().postProcessData;
    pp.vignette_smoothness = vignetteFrom * (1.0f - e) + vignetteTo * e;
}