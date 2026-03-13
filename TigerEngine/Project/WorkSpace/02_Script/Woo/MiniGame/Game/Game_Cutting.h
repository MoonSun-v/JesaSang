#pragma 
#include "IMinigame.h"
#include <directxtk/Keyboard.h>

using namespace DirectX;
class RectTransform;
class Image;
class GameObject;


/*
    [ 칼로 썰기 게임 ]
    
    메이플 스타포스임
    Space 누르면 슬라이더(칼)가 멈추고 판정, 일정 시간 뒤 다시 움직임
    슬라이더 움직이는 영역(x) 200 ~ 800
    StopPoint : pos.x = 500, size.x = 100
    StopPoint 영역 안에 Knife 중심이 들어오면 성공으로 판정

    - 게임 성공 : 5번 성공
    - 게임 실패 : 3번 실패
*/

class Game_Cutting : public IMiniGame
{
private:
    // UI
    RectTransform* stopPoint;       // 맞춰야하는 부분(영역)
    RectTransform* knife;           // 움직이는 슬라이더 (space key down시 정지)

    RectTransform* feedback_good;   // 텀 피드백
    RectTransform* feedback_bad;    // 텀 피드백

    GameObject* spacebarHold = nullptr;

    // game data
    float spaceSpped = 700.0f;      // 스페이스바 슬라이드 속도
    float spaceStopTime = 1.0f;     // 정지했을때 멈춰있는 시간
    Keyboard::Keys spaceKey = Keyboard::Space;

    // controll data
    // success/fail
    int curFailCount = 0;       // 현재까지 실패 횟수 ( >=3 면 게임 종료)
    int curSuccessCount = 0;    // 현재까지 성공 횟수 (>= 5면 게임 종료)

    // slider movement
    float xMin = 200.0f;
    float xMax = 800.0f;
    float dir = 1.0f;     // +1 오른쪽, -1 왼쪽

    // state
    bool  isStopped = false;         // 멈춘 상태인지
    float stopTimer = 0.0f;          // 멈춘 뒤 경과시간
    bool  consumedThisStop = false;  // 멈춘 상태에서 판정 1회

    // 텀 피드백 연출
    bool isFeedbackPlaying = false;
    RectTransform* currentFeedback = nullptr;

    float feedbackTimer = 0.0f;
    float feedbackDuration = 0.25f;

    float feedbackStartY = -150.0f;
    float feedbackEndY = -200.0f;


    // funcs..
    float GetUIPosX(RectTransform* rt) const;
    void  SetUIPosX(RectTransform* rt, float x);
    float GetUIWidth(RectTransform* rt) const;

    bool  IsSpaceDownOnce() const; 
    void  EvaluateOnce();         

    void ShowFeedback(bool isGood);

    Image* animImage{};
    void SetAnimPathes();

public:
    Game_Cutting() = default;
    ~Game_Cutting() override = default;

    void StartGame() override;
    void UpdateGame(float delta)override;
    void EndGame() override;

    void UpdateAnimation(float delta) override;
};

