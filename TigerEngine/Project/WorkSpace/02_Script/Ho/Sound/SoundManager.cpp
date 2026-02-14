#include "SoundManager.h"
#include "../../../Engine/Object/GameObject.h"
#include "../../../Engine/Util/ComponentAutoRegister.h"
#include "../../../Engine/Util/JsonHelper.h"

REGISTER_COMPONENT(SoundManager)

RTTR_REGISTRATION
{
    rttr::registration::class_<SoundManager>("SoundManager")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

void SoundManager::OnInitialize()
{
    // 중복 생성 방지
    if (s_instance != nullptr && s_instance != this)
    {
        assert(false && "Duplicate GameManager instance detected.");
        return;
    }

    s_instance = this;
}

void SoundManager::OnStart()
{
    auto comps = GetOwner()->GetComponents();

    bgmSource = nullptr;
    sfxSources.clear();

    for (auto* c : comps)
    {
        if (!c) continue;
        if (c->GetName() == "AudioSourceComponent")
        {
            auto* src = static_cast<AudioSourceComponent*>(c); 
            if (!bgmSource) bgmSource = src;      // 첫 오디오 소스를 BGM으로
            else sfxSources.push_back(src);       // 나머지 SFX로
        }
    }
}

void SoundManager::OnDestory()
{
    if (s_instance == this) s_instance = nullptr;
}

void SoundManager::PlayBGM(BGMType type, bool restart)
{
    std::string id = ToString(type);

    bgmSource->SetLoop(restart);
    bgmSource->SetClipId(id);
    bgmSource->Play();
}

void SoundManager::StopBGM()
{
    bgmSource->Stop();
}

void SoundManager::PauseBGM(bool paused)
{
    bgmSource->Pause(paused);
}

void SoundManager::PlaySFX(SFXType type)
{
    auto id = ToString(type);

    // 비어있는 소스 찾기
    AudioSourceComponent* freeSrc = nullptr;
    for (auto* src : sfxSources)
    {
        if (src && !src->IsPlaying())
        {
            freeSrc = src;
            break;
        }
    }

    if (freeSrc)
    {
        freeSrc->SetLoop(false);
        freeSrc->SetClipId(id);
        freeSrc->Play();
    }
}

nlohmann::json SoundManager::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void SoundManager::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

std::string SoundManager::ToString(BGMType type)
{
    switch (type)
    {
    case BGMType::Main:          return "Main";
    case BGMType::InGame_BG:     return "InGame_BG";
    case BGMType::Minigame_BG:   return "Minigame_BG";

        // 추가 BG
    case BGMType::Ghost_Chase_BG: return "Ghost_Chase_BG";
    case BGMType::Altar_BG:       return "Altar_BG";
    case BGMType::Game_Win_BG:    return "Game_Win_BG";
    case BGMType::Game_Lose_BG:   return "Game_Lose_BG";

    default: return "";
    }
}


std::string SoundManager::ToString(SFXType type)
{
    switch (type)
    {
    case SFXType::HiddenObj_Playerin_Sound:           return "HiddenObj_Playerin_Sound";
    case SFXType::HiddenObj_Playerout_Sound:          return "HiddenObj_Playerout_Sound";
    case SFXType::HiddenObj_Nointeraction_Sound:      return "HiddenObj_Nointeraction_Sound";
    case SFXType::FindObj_Interaction_1_Sound:        return "FindObj_Interaction_1_Sound";
    case SFXType::FindObj_Interaction_2_Sound:        return "FindObj_Interaction_2_Sound";
    case SFXType::FindObj_Acquiremap_Sound:           return "FindObj_Acquiremap_Sound";
    case SFXType::FindObj_Acquireitem_Sound:          return "FindObj_Acquireitem_Sound";
    case SFXType::PitfallObj_Floor_Sound:             return "PitfallObj_Floor_Sound";
    case SFXType::PitfallObj_Table_Sound:             return "PitfallObj_Table_Sound";
    case SFXType::PitfallObj_SleepGhost_Sound:        return "PitfallObj_SleepGhost_Sound";
    case SFXType::PitfallObj_SleepGhost_Wakeup_Sound: return "PitfallObj_SleepGhost_Wakeup_Sound";
    case SFXType::GoalObj_Sound:                      return "GoalObj_Sound";
       
    // ---- Ghost ----
    case SFXType::Ghost_Move_Sound:                    return "Ghost_Move_Sound";
    case SFXType::Ghost_Frozen_Sound:                  return "Ghost_Frozen_Sound";
    case SFXType::Ghost_Playerfind_Sound:              return "Ghost_Playerfind_Sound";
    case SFXType::Ghost_Attack_Sound:                  return "Ghost_Attack_Sound";
    case SFXType::Ghost_AttackDelay_Sound:             return "Ghost_AttackDelay_Sound";

     // ---- BabyGhost ----
    case SFXType::BabyGhost_Move_Sound:                return "BabyGhost_Move_Sound";
    case SFXType::BabyGhost_Playerfind_Sound:          return "BabyGhost_Playerfind_Sound";
    case SFXType::BabyGhost_Crying_Sound:              return "BabyGhost_Crying_Sound";

     // ---- Altar ----
    case SFXType::Altar_Fire_Sound:                    return "Altar_Fire_Sound";
    case SFXType::Altar_Mapchange_Sound:               return "Altar_Mapchange_Sound";

    // ---- Minigame ----
    case SFXType::Minigame_Interaction_Sound:          return "Minigmae_Interaction_Sound";
    case SFXType::Minigame_Clear_Sound:                return "Minigmae_Clear_Sound";
    case SFXType::Minigame_Success_Sound:              return "Minigmae_Success_Sound";
    case SFXType::Minigame_Fail_Sound:                 return "Minigmae_Fail_Sound";
    case SFXType::Minigame_Wrong_Sound:                return "Minigmae_Wrong_Sound";
    }
    return "SFX_Unknown";
}
