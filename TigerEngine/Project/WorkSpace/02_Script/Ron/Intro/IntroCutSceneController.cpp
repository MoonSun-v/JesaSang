#include "IntroCutSceneController.h"

#include <algorithm>
#include <array>
#include <filesystem>

#include "Components/AudioSourceComponent.h"
#include "Components/RectTransform.h"
#include "Components/UI/Image.h"
#include "EngineSystem/SceneSystem.h"
#include "Manager/UIManager.h"
#include "Object/GameObject.h"
#include "Util/ComponentAutoRegister.h"
#include "Util/JsonHelper.h"
#include "Util/PathHelper.h"
#include "System/InputSystem.h"
#include "../UI/SettingsUIController.h"

REGISTER_COMPONENT(IntroCutSceneController);

RTTR_REGISTRATION
{
    rttr::registration::class_<IntroCutSceneController>("IntroCutSceneController")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr)
        .property("ImageObjectName", &IntroCutSceneController::imageObjectName)
        .property("FrameObjectName", &IntroCutSceneController::frameObjectName)
        .property("NextScenePath", &IntroCutSceneController::nextScenePath)
        .property("AllowSkip", &IntroCutSceneController::allowSkip);
}

namespace
{
    constexpr float kPreSlideFrameInterval = 0.2f;
    constexpr float kPreSlideSize = 500.0f;
    constexpr const char* kBlackOverlayPath = "..\\Assets\\Resource\\CutScene\\intro-black-overlay.png";
    constexpr const char* kCutSceneFramePath = "..\\Assets\\Resource\\CutScene\\cutscene-frame.png";
    const std::array<const char*, 8> kPreSlideFileNames =
    {
        "00.png", "01.png", "02.png", "03.png", "04.png", "05.png", "06.png", "07.png"
    };
    const std::array<const char*, 3> kPreSlideSoundIds =
    {
        "Intro_Gamestart_1_Sound",
        "Intro_Gamestart_2_Sound",
        "Intro_Gamestart_3_Sound"
    };
    constexpr const char* kMainIntroStartSoundId = "Intro_Gamestart_4_Sound";
    constexpr const char* kTitleBgmClipId = "Intro_Title_BG";

    struct FrameEntry
    {
        int major = 0;
        int minor = 0;
        std::string filename;
    };

    bool ParseFrameName(const std::string& filename, FrameEntry& out)
    {
        const std::string prefixEn = "intro_";
        const std::string prefixKr = "인트로_";
        std::string usedPrefix;

        if (filename.rfind(prefixEn, 0) == 0)
        {
            usedPrefix = prefixEn;
        }
        else if (filename.rfind(prefixKr, 0) == 0)
        {
            usedPrefix = prefixKr;
        }
        else
        {
            return false;
        }

        std::string stem = filename;
        const auto dotPos = stem.find_last_of('.');
        if (dotPos != std::string::npos)
        {
            stem = stem.substr(0, dotPos);
        }

        std::string numberPart = stem.substr(usedPrefix.size());
        int major = 0;
        int minor = 0;
        const auto splitPos = numberPart.find('.');
        try
        {
            if (splitPos == std::string::npos)
            {
                major = std::stoi(numberPart);
            }
            else
            {
                major = std::stoi(numberPart.substr(0, splitPos));
                minor = std::stoi(numberPart.substr(splitPos + 1));
            }
        }
        catch (...)
        {
            return false;
        }

        out.major = major;
        out.minor = minor;
        out.filename = filename;
        return true;
    }

    bool IsAdvancePressed(bool current, bool& prev)
    {
        const bool pressed = current && !prev;
        prev = current;
        return pressed;
    }
}

void IntroCutSceneController::OnInitialize()
{
}

void IntroCutSceneController::OnStart()
{
    auto scene = SceneSystem::Instance().GetCurrentScene();
    if (!scene)
    {
        return;
    }

    if (auto imageObj = scene->GetGameObjectByName(imageObjectName))
    {
        m_Image = imageObj->GetComponent<Image>();
        m_ImageRect = imageObj->GetComponent<RectTransform>();
    }

    if (auto frameObj = scene->GetGameObjectByName(frameObjectName))
    {
        m_FrameImage = frameObj->GetComponent<Image>();
        m_FrameRect = frameObj->GetComponent<RectTransform>();
    }

    LoadFrameList();
    LoadPreFrameList();
    EnsureAudioSource();

    if (!m_PreFrames.empty())
    {
        StartPreSequence();
    }
    else
    {
        StartMainSequence();
    }
}

void IntroCutSceneController::OnUpdate(float delta)
{
    if (m_State == SequenceState::PreSlide)
    {
        m_PreFrameAccumulator += delta;
        while (m_PreFrameAccumulator >= kPreSlideFrameInterval)
        {
            m_PreFrameAccumulator -= kPreSlideFrameInterval;
            const int nextIndex = (m_PreFrameIndex + 1) % static_cast<int>(m_PreFrames.size());
            ApplyPreFrame(nextIndex);
        }

        if (m_PreSoundIndex < kPreSlideSoundIds.size())
        {
            if (!m_AudioSource || !m_AudioSource->IsPlaying())
            {
                ++m_PreSoundIndex;
                if (m_PreSoundIndex < kPreSlideSoundIds.size())
                {
                    PlayClip(kPreSlideSoundIds[m_PreSoundIndex], false);
                }
                else
                {
                    StartMainSequence();
                }
            }
        }

        return;
    }

    if (!m_LayoutApplied)
    {
        ApplyMainLayout();
    }

    if (!m_TitleBgmStarted && m_AudioSource && !m_AudioSource->IsPlaying())
    {
        PlayClip(kTitleBgmClipId, true);
        m_TitleBgmStarted = true;
    }

    const bool spaceDown = Input::GetKey(DirectX::Keyboard::Keys::Space);
    const bool enterDown = Input::GetKey(DirectX::Keyboard::Keys::Enter);
    const bool escDown = Input::GetKey(DirectX::Keyboard::Keys::Escape);

    // While settings UI is open, block intro advance/skip input propagation.
    if (auto* settings = SettingsUIController::Instance())
    {
        if (settings->IsOpen())
        {
            m_PrevSpace = spaceDown;
            m_PrevEnter = enterDown;
            m_PrevEsc = escDown;
            return;
        }
    }

    const bool advance = IsAdvancePressed(spaceDown, m_PrevSpace)
        || IsAdvancePressed(enterDown, m_PrevEnter);

    if (advance)
    {
        const int nextIndex = m_CurrentIndex + 1;
        if (nextIndex < static_cast<int>(m_Frames.size()))
        {
            ApplyFrame(nextIndex);
        }
        else
        {
            LoadNextScene();
        }
    }

    if (allowSkip && IsAdvancePressed(escDown, m_PrevEsc))
    {
        LoadNextScene();
    }
}

nlohmann::json IntroCutSceneController::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void IntroCutSceneController::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void IntroCutSceneController::LoadFrameList()
{
    m_Frames.clear();

    auto dir = PathHelper::FindDirectory("Assets/Resource/CutScene");
    if (!dir)
    {
        return;
    }

    std::vector<FrameEntry> entries;
    for (const auto& entry : std::filesystem::directory_iterator(*dir))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const auto filename = entry.path().filename().string();
        if (filename == "타이틀.png"
            || filename == "컷씬-프레임.png"
            || filename == "title.png"
            || filename == "cutscene-frame.png")
        {
            continue;
        }

        FrameEntry frame;
        if (ParseFrameName(filename, frame))
        {
            entries.push_back(frame);
        }
    }

    std::sort(entries.begin(), entries.end(), [](const FrameEntry& a, const FrameEntry& b)
    {
        if (a.major != b.major) return a.major < b.major;
        return a.minor < b.minor;
    });

    const std::string base = "..\\Assets\\Resource\\CutScene\\";
    for (const auto& entry : entries)
    {
        m_Frames.push_back(base + entry.filename);
    }
}

void IntroCutSceneController::LoadPreFrameList()
{
    m_PreFrames.clear();

    auto dir = PathHelper::FindDirectory("Assets/Resource/CutScene");
    if (!dir)
    {
        return;
    }

    const std::string base = "..\\Assets\\Resource\\CutScene\\";
    for (const char* fileName : kPreSlideFileNames)
    {
        const auto fullPath = std::filesystem::path(*dir) / fileName;
        if (std::filesystem::exists(fullPath))
        {
            m_PreFrames.push_back(base + fileName);
        }
    }
}

void IntroCutSceneController::ApplyFrame(int index)
{
    if (!m_Image || index < 0 || index >= static_cast<int>(m_Frames.size()))
    {
        return;
    }

    m_CurrentIndex = index;
    m_Image->ChangeData(m_Frames[index]);
}

void IntroCutSceneController::ApplyPreFrame(int index)
{
    if (!m_Image || index < 0 || index >= static_cast<int>(m_PreFrames.size()))
    {
        return;
    }

    m_PreFrameIndex = index;
    m_Image->ChangeData(m_PreFrames[index]);
}

void IntroCutSceneController::ApplyLayout()
{
    if (m_State == SequenceState::PreSlide)
    {
        ApplyPreLayout();
    }
    else
    {
        ApplyMainLayout();
    }
}

void IntroCutSceneController::ApplyPreLayout()
{
    Vector2 screen = UIManager::Instance().GetSize();
    if (screen.x <= 0.0f || screen.y <= 0.0f)
    {
        return;
    }

    if (m_ImageRect)
    {
        m_ImageRect->SetPivot({ 0.0f, 0.0f });
        const float x = (screen.x - kPreSlideSize) * 0.5f;
        const float y = (screen.y - kPreSlideSize) * 0.5f;
        m_ImageRect->SetPos({ x, y, 0.0f });
        m_ImageRect->SetSize({ kPreSlideSize, kPreSlideSize });
    }

    if (m_FrameRect)
    {
        m_FrameRect->SetPivot({ 0.0f, 0.0f });
        m_FrameRect->SetPos({ 0.0f, 0.0f, 0.0f });
        m_FrameRect->SetSize(screen);
    }

    if (m_FrameImage)
    {
        m_FrameImage->ChangeData(kBlackOverlayPath);
        m_FrameImage->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        m_FrameImage->SetZOrder(0);
    }

    if (m_Image)
    {
        m_Image->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        m_Image->SetZOrder(1);
    }

    m_LayoutApplied = true;
}

void IntroCutSceneController::ApplyMainLayout()
{
    Vector2 screen = UIManager::Instance().GetSize();
    if (screen.x <= 0.0f || screen.y <= 0.0f)
    {
        return;
    }

    if (m_ImageRect)
    {
        m_ImageRect->SetPivot({ 0.0f, 0.0f });
        m_ImageRect->SetPos({ 0.0f, 0.0f, 0.0f });
        m_ImageRect->SetSize(screen);
    }

    if (m_FrameRect)
    {
        m_FrameRect->SetPivot({ 0.0f, 0.0f });
        m_FrameRect->SetPos({ 0.0f, 0.0f, 0.0f });
        m_FrameRect->SetSize(screen);
    }

    if (m_FrameImage)
    {
        m_FrameImage->ChangeData(kCutSceneFramePath);
        m_FrameImage->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        m_FrameImage->SetZOrder(2);
    }

    if (m_Image)
    {
        m_Image->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
        m_Image->SetZOrder(1);
    }

    m_LayoutApplied = true;
}

void IntroCutSceneController::EnsureAudioSource()
{
    auto* owner = GetOwner();
    if (!owner)
    {
        return;
    }

    m_AudioSource = owner->GetComponent<AudioSourceComponent>();
    if (!m_AudioSource)
    {
        m_AudioSource = owner->AddComponent<AudioSourceComponent>();
    }

    if (m_AudioSource)
    {
        m_AudioSource->SetLoop(false);
    }
}

void IntroCutSceneController::PlayClip(const std::string& clipId, bool loop)
{
    if (!m_AudioSource || clipId.empty())
    {
        return;
    }

    m_AudioSource->SetChannelGroup(loop ? "BGM" : "SFX");
    m_AudioSource->SetLoop(loop);
    m_AudioSource->SetClipId(clipId);
    m_AudioSource->Play(true);
}

void IntroCutSceneController::StartPreSequence()
{
    m_State = SequenceState::PreSlide;
    m_LayoutApplied = false;
    m_PreFrameAccumulator = 0.0f;
    m_PreFrameIndex = 0;
    m_PreSoundIndex = 0;

    ApplyPreLayout();
    ApplyPreFrame(0);

    if (!kPreSlideSoundIds.empty())
    {
        PlayClip(kPreSlideSoundIds[0], false);
    }
}

void IntroCutSceneController::StartMainSequence()
{
    m_State = SequenceState::MainCutScene;
    m_LayoutApplied = false;
    m_PrevSpace = Input::GetKey(DirectX::Keyboard::Keys::Space);
    m_PrevEnter = Input::GetKey(DirectX::Keyboard::Keys::Enter);
    m_PrevEsc = Input::GetKey(DirectX::Keyboard::Keys::Escape);

    ApplyMainLayout();
    if (!m_Frames.empty())
    {
        ApplyFrame(0);
    }

    m_TitleBgmStarted = false;
    PlayClip(kMainIntroStartSoundId, false);
}

void IntroCutSceneController::LoadNextScene()
{
    if (nextScenePath.empty())
    {
        return;
    }

    if (auto scene = SceneSystem::Instance().GetCurrentScene())
    {
        nextScenePath = "..\\Assets\\Scenes\\03_TutorialScene.json";
        scene->LoadToJson(nextScenePath);
    }
}
