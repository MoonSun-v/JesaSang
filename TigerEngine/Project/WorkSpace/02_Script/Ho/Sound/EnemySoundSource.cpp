#include "EnemySoundSource.h"
#include "Object/GameObject.h"
#include "../../../Engine/Util/ComponentAutoRegister.h"
#include "../../../Engine/Util/JsonHelper.h"

REGISTER_COMPONENT(EnemySoundSource)

RTTR_REGISTRATION
{
    rttr::registration::class_<EnemySoundSource>("EnemySoundSource")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}
void EnemySoundSource::OnStart()
{
    sound = GetOwner()->GetComponent<AudioSourceComponent>();

    if (!sound) // 없으면 생성후 연결
    {
        sound = GetOwner()->AddComponent<AudioSourceComponent>();
    }
}

void EnemySoundSource::OnUpdate(float dt)
{
    UpdateAudioTransform(dt);
}

nlohmann::json EnemySoundSource::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void EnemySoundSource::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void EnemySoundSource::PlaySound(EnemySoundType type, bool loop)
{
    if (!sound) return;
    std::string id = "";

    switch (type)
    {
    case EnemySoundType::Ghost_Move_Sound:
        id = "Ghost_Move_Sound";
        break;
    case EnemySoundType::Ghost_Frozen_Sound:
        id = "Ghost_Frozen_Sound";
        break;
    case EnemySoundType::Ghost_Playerfind_Sound:
        id = "Ghost_Playerfind_Sound";
        break;
    case EnemySoundType::Ghost_Attack_Sound:
        id = "Ghost_Attack_Sound";
        break;
    case EnemySoundType::Ghost_AttackDelay_Sound:
        id = "Ghost_AttackDelay_Sound";
        break;
    case EnemySoundType::BabyGhost_Move_Sound:
        id = "BabyGhost_Move_Sound";
        break;
    case EnemySoundType::BabyGhost_Playerfind_Sound:
        id = "BabyGhost_Playerfind_Sound";
        break;
    case EnemySoundType::BabyGhost_Crying_Sound:
        id = "BabyGhost_Crying_Sound";
        break;
    default:
        break;
    }

    sound->SetLoop(loop);
    sound->SetClipId(id);
    sound->Play();
}

void EnemySoundSource::UpdateAudioTransform(float delta)
{
    if (!sound) return;

    auto pos = GetOwner()->GetTransform()->GetWorldPosition();
    auto fwd = GetOwner()->GetTransform()->GetForward();
    auto up = GetOwner()->GetTransform()->GetUp();
    Vector3 vel = Vector3::Zero;
    if (hasPrev && delta > 0.0001f)
        vel = (pos - prevPos) / delta;

    prevPos = pos;
    hasPrev = true;

    AudioTransform t{};
    t.position = { pos.x, pos.y, pos.z };       // 위치
    t.forward = { fwd.x, fwd.y, fwd.z };        // forward
    t.up = { up.x,  up.y,  up.z };              // up vector
    t.velocity = { vel.x, vel.y, vel.z };       // "초당 이동량"(world-space). doppler 등에 사용됨. ( 청자(Listener)가 얼마나 / 어느 방향으로 움직이는지 알려주는 값 
    
    sound->SetFallback(t); // AudioListenerComponent가 Update()에서 적용(Engine/Components/AudioListenerComponent.cpp:61)
}
