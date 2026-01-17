#pragma once
#include "Singleton.h"

enum class PlayModeState
{
    Stopped,    // editor 모드
    Playing,    // play 모드
    Paused,      // 일시 정지
};

class PlayModeSystem : public Singleton<PlayModeSystem>
{
public:
    PlayModeSystem(token) : state(PlayModeState::Stopped) {} // 처음 시작은 Editor모드
    ~PlayModeSystem() = default;

    void SetPlayMode(PlayModeState newState) { state = newState; }
    PlayModeState GetPlayMode() const { return state; }
    bool IsPlaying() const { return state == PlayModeState::Playing; }

private:
    PlayModeState state;
};

