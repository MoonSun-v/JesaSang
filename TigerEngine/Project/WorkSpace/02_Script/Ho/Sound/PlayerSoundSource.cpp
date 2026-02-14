#include "PlayerSoundSource.h"
#include "Object/GameObject.h"
#include "../../../Engine/Util/ComponentAutoRegister.h"
#include "../../../Engine/Util/JsonHelper.h"
#include <random>

REGISTER_COMPONENT(PlayerSoundSource)

RTTR_REGISTRATION
{
    rttr::registration::class_<PlayerSoundSource>("PlayerSoundSource")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

void PlayerSoundSource::OnStart()
{
    // AudioSourceComponent를 owner에서 싹 긁어서 2개 확보
    sound = nullptr;
    footSound = nullptr;

    auto comps = GetOwner()->GetComponents();
    std::vector<AudioSourceComponent*> audios;
    audios.reserve(4);

    for (auto* c : comps)
    {
        if (!c) continue;
        if (c->GetName() == "AudioSourceComponent")
            audios.push_back(static_cast<AudioSourceComponent*>(c));
    }

    if (audios.size() >= 2)
    {
        sound = audios[0];
        footSound = audios[1];
    }
    else if (audios.size() == 1)
    {
        sound = audios[0];
        footSound = GetOwner()->AddComponent<AudioSourceComponent>(); // 한 개 추가
    }
    else
    {
        sound = GetOwner()->AddComponent<AudioSourceComponent>();
        footSound = GetOwner()->AddComponent<AudioSourceComponent>(); // 두 개 생성
    }

    // 발소리는 항상 원샷 재생(타이머 루프)로 갈 거라 loop false 고정 권장
    if (footSound) footSound->SetLoop(false);

    std::cout << "[PlayerSoundSource] AudioSource ready. action=" << (sound ? "OK" : "NULL")
        << ", foot=" << (footSound ? "OK" : "NULL") << std::endl;
}

void PlayerSoundSource::OnUpdate(float delta)
{
    if (!footLoopPlaying || !footSound) return;

    stepTimer -= delta;
    if (stepTimer > 0.0f) return;

    if (footSound->IsPlaying() && stepInterval >= 0.2f)
    {
        stepTimer = 0.01f;
        return;
    }

    PlayOneFootStep();
    stepTimer = stepInterval;
}

nlohmann::json PlayerSoundSource::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void PlayerSoundSource::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void PlayerSoundSource::PlaySound(PlayerSoundType type, bool loop)
{
    if (!sound) return;
    std::string id = "";

    // NOTE id는 csv에서 정해짐
    switch (type)
    {
    case PlayerSoundType::Sit:
        id = "Player_Sit_Sound";
        break;
    case PlayerSoundType::Walk:
        id = "Player_Walk_Sound";
        break;
    case PlayerSoundType::Run:
        id = "Player_Run_Sound";
        break;
    case PlayerSoundType::Hit:
        id = "Player_Hit_Sound";
        break;
    case PlayerSoundType::Confused:
        id = "Player_Confused_Sound";
        break;
    case PlayerSoundType::HpDown:
        id = "Player_HpDown_Sound";
        break;
    case PlayerSoundType::Typing:
        id = "Player_Typing_Sound";
        break;
    default:
        break;
    }

    if (id.empty()) return;

    sound->SetLoop(loop);
    sound->SetClipId(id);
    sound->Play();

    std::cout << "Play Sound : " << id << std::endl;
}

void PlayerSoundSource::StopSound()
{
    if (!sound) return;
    sound->Stop();
}

void PlayerSoundSource::PlayRandomFootStep()
{
    if (!footSound) return;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, static_cast<int>(PlayerFootStepType::Player_Walk_Sound_Left));

    PlayerFootStepType type = static_cast<PlayerFootStepType>(dist(gen));

    std::string id;
    switch (type)
    {
    case PlayerFootStepType::Player_Walk_Sound_Right: id = "Player_Walk_Sound_Right"; break;
    case PlayerFootStepType::Player_Walk_Sound_Left:  id = "Player_Walk_Sound_Left"; break;
    default: return;
    }

    if (id.empty()) return;

    footSound->SetLoop(false);
    footSound->SetClipId(id);
    footSound->Play();
}

void PlayerSoundSource::PlayFootStepLoop(FootStepLoopMode mode)
{
    if (!footSound) return;

    footLoopPlaying = true;
    stepInterval = GetIntervalByMode(mode);

    // Enter에서 호출하면 바로 1발 나가고 시작하도록
    stepTimer = 0.0f;

    // 첫 발 어느쪽부터 시작할지 취향 (원하면 true 고정)
    // nextStepRight = true;

    // 혹시 이전 상태에서 남아있던 소리 끊고 시작
    footSound->Stop();
}

void PlayerSoundSource::StopFootStep()
{
    footLoopPlaying = false;
    stepTimer = 0.0f;

    if (footSound)
        footSound->Stop();
}

float PlayerSoundSource::GetIntervalByMode(FootStepLoopMode mode) const
{
    switch (mode)
    {
    case FootStepLoopMode::SlowWalk: return 0.75f;
    case FootStepLoopMode::Walk:     return 0.38f;
    case FootStepLoopMode::Run:      return 0.15f;
    default:                         return 0.38f;
    }
}

void PlayerSoundSource::PlayOneFootStep()
{
    if (!footSound) return;

    std::string id = nextStepRight ? "Player_Walk_Sound_Right" : "Player_Walk_Sound_Left";
    nextStepRight = !nextStepRight;

    if (id.empty()) return;

    footSound->Stop();
    footSound->SetLoop(false);
    footSound->SetClipId(id);
    footSound->Play();
}
