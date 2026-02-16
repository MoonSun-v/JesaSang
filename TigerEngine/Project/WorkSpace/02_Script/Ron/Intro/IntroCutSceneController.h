#pragma once

#include "Components/ScriptComponent.h"
#include <directxtk/Keyboard.h> // x

class Image;
class RectTransform;
class AudioSourceComponent;

class IntroCutSceneController : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)
public:
    IntroCutSceneController() { SetName("IntroCutSceneController"); }
    ~IntroCutSceneController() override = default;

    void OnInitialize() override;
    void OnStart() override;
    void OnUpdate(float delta) override;

    nlohmann::json Serialize() override;
    void Deserialize(nlohmann::json data) override;

    std::string imageObjectName = "UI_Intro_Image";
    std::string frameObjectName = "UI_Intro_Frame";
    std::string nextScenePath = "..\\Assets\\Scenes\\03_TutorialScene.json";

    bool allowSkip = true;

private:
    void LoadFrameList();
    void LoadPreFrameList();
    void ApplyFrame(int index);
    void ApplyPreFrame(int index);
    void ApplyLayout();
    void ApplyPreLayout();
    void ApplyMainLayout();
    void EnsureAudioSource();
    void PlayClip(const std::string& clipId, bool loop);
    void StartPreSequence();
    void StartMainSequence();
    void LoadNextScene();

    enum class SequenceState
    {
        PreSlide,
        MainCutScene,
    };

    Image* m_Image = nullptr;
    RectTransform* m_ImageRect = nullptr;
    Image* m_FrameImage = nullptr;
    RectTransform* m_FrameRect = nullptr;
    AudioSourceComponent* m_AudioSource = nullptr;

    std::vector<std::string> m_Frames;
    std::vector<std::string> m_PreFrames;
    int m_CurrentIndex = 0;
    int m_PreFrameIndex = 0;
    float m_PreFrameAccumulator = 0.0f;
    size_t m_PreSoundIndex = 0;
    bool m_TitleBgmStarted = false;

    SequenceState m_State = SequenceState::MainCutScene;
    bool m_LayoutApplied = false;
    bool m_PrevSpace = false;
    bool m_PrevEnter = false;
    bool m_PrevEsc = false;
};
