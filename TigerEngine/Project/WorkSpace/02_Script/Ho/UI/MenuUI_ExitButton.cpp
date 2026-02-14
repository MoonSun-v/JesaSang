#include "MenuUI_ExitButton.h"

#include "../Engine/Util/JsonHelper.h"
#include "../Engine/Object/GameObject.h"
#include "../Base/Datas/ReflectionMedtaDatas.hpp"
#include "../Engine/Util/ComponentAutoRegister.h"
#include "../Engine/Components/AudioSourceComponent.h"


namespace
{
    constexpr const char* kExitButtonClickClipId = "Intro_Button_Sound";
}

REGISTER_COMPONENT(MenuUI_ExitButton);

RTTR_REGISTRATION
{
    rttr::registration::class_<MenuUI_ExitButton>("MenuUI_ExitButton")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr)
        .property("pressedImagePath", &MenuUI_ExitButton::GetPressImagePath, &MenuUI_ExitButton::SetPressImagePath)
            (metadata(META_BROWSE, "")) // 탐색
        .property("normalImagePath", &MenuUI_ExitButton::normalImagePath)
        .property("targetScenePath", &MenuUI_ExitButton::targetScenePath)
            (metadata(META_BROWSE, ""));
}

void MenuUI_ExitButton::OnInitialize() {}
void MenuUI_ExitButton::OnEnable() {}
void MenuUI_ExitButton::OnDisable() {}
void MenuUI_ExitButton::OnDestory() {}

void MenuUI_ExitButton::OnStart()
{
    auto* owner = GetOwner();
    if (!owner)
        return;

    image = owner->GetComponent<Image>();
    EnsureClickAudioSource();

    if (!image)
        return;

    // 최초 normal 경로 저장
    normalImagePath = image->GetPath();

    // 눌렀을 때
    image->OnPressed.AddListener(image, [this]()
        {
            if (!pressedImagePath.empty())
            {
                image->ChangeData(pressedImagePath);
            }
        });

    // 뗐을 때
    image->OnPressOut.AddListener(image, [this]()
        {
            if (!normalImagePath.empty())
            {
                image->ChangeData(normalImagePath);
            }

            PlayClickSound();
            exit(0);
            // TODO: 씬 전환 로직 연결
            // SceneManager::Get().LoadScene(targetScenePath);
        });
}

void MenuUI_ExitButton::OnUpdate(float /*delta*/)
{
    if (!image)
        return;
}

nlohmann::json MenuUI_ExitButton::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void MenuUI_ExitButton::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

const std::string& MenuUI_ExitButton::GetPressImagePath() const
{
    return pressedImagePath;
}

void MenuUI_ExitButton::SetPressImagePath(const std::string& path)
{
    pressedImagePath = path;
}

void MenuUI_ExitButton::EnsureClickAudioSource()
{
    auto* owner = GetOwner();
    if (!owner)
        return;

    m_ClickAudioSource = owner->GetComponent<AudioSourceComponent>();
    if (!m_ClickAudioSource)
    {
        m_ClickAudioSource = owner->AddComponent<AudioSourceComponent>();
    }
}

void MenuUI_ExitButton::PlayClickSound()
{
    if (!m_ClickAudioSource)
        return;

    m_ClickAudioSource->SetChannelGroup("SFX");
    m_ClickAudioSource->SetLoop(false);
    m_ClickAudioSource->SetClipId(kExitButtonClickClipId);
    m_ClickAudioSource->Play(true);
}
