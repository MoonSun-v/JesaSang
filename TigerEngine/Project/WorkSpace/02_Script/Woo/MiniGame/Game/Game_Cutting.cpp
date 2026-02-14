#include "Game_Cutting.h"
#include <iostream>
#include <algorithm>

#include "EngineSystem/SceneSystem.h"
#include "System/InputSystem.h"
#include "Object/GameObject.h"
#include "Components/RectTransform.h"
#include "Components/UI/Image.h"
#include <algorithm>
#include "../../../Ho/Sound/SoundManager.h"

using std::cout;
using std::endl;


float Game_Cutting::GetUIPosX(RectTransform* rt) const
{
    return rt->GetPos().x;
}

void Game_Cutting::SetUIPosX(RectTransform* rt, float x)
{
    Vector3 pos = rt->GetPos();
    pos.x = x;
    rt->SetPos(pos);
}

float Game_Cutting::GetUIWidth(RectTransform* rt) const
{
    return rt->GetSize().x;
}

bool Game_Cutting::IsSpaceDownOnce() const
{
    return InputSystem::Instance().GetKeyDown(spaceKey);
}

void Game_Cutting::EvaluateOnce()
{
    const float stopX = GetUIPosX(stopPoint);
    const float stopW = std::max(1.0f, GetUIWidth(stopPoint));

    const float barX = GetUIPosX(knife);
    const float barW = std::max(1.0f, GetUIWidth(knife));

    const float stopLeft = stopX - stopW * 0.5f;
    const float stopRight = stopX + stopW * 0.5f;

    const float barCenter = barX;
  
    const bool hit = (barCenter >= stopLeft && barCenter <= stopRight);

    if (hit)
    {
        curSuccessCount++;
        SoundManager::Instance()->PlaySFX(SFXType::Minigame_Success_Sound);
        cout << "[MiniGame Cutting] SUCCESS! (" << curSuccessCount << "/5)" << endl;
    }
    else
    {
        curFailCount++;
        SoundManager::Instance()->PlaySFX(SFXType::Minigame_Wrong_Sound);
        cout << "[MiniGame Cutting] FAIL! (" << curFailCount << "/3)" << endl;
    }

    // 성공/실패 체크
    if (curSuccessCount >= 5)
    {
        isFinished = true;
        isSuccess = true;
        SoundManager::Instance()->PlaySFX(SFXType::Minigame_Clear_Sound);
    }
    else if (curFailCount >= 3)
    {
        isFinished = true;
        isSuccess = false;
        SoundManager::Instance()->PlaySFX(SFXType::Minigame_Fail_Sound);
    }
}

void Game_Cutting::StartGame()
{
    stopPoint = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("1_Image_StopPoint")->GetComponent<RectTransform>();
    knife = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("1_Image_Knife")->GetComponent<RectTransform>();
    animImage = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("1_Image_CutAnimation")->GetComponent<Image>();

    spacebarHold = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Image_SpaceBarHold");

    if (!stopPoint || !knife || !spacebarHold)
    {
        cout << "[MiniGame 1] Missing ui objects!" << endl;
        return;
    }

    // init
    timer = 0.0f;
    curFailCount = 0;
    curSuccessCount = 0;

    dir = 1.0f;
    isStopped = false;
    stopTimer = 0.0f;
    consumedThisStop = false;

    // start pos init
    SetUIPosX(knife, (xMin + xMax) * 0.5f);

    // anim image set
    SetAnimPathes();

    // hold ui off
    spacebarHold->SetActive(false);

    std::cout << "[MiniGame 1] : Game_Cutting Start!" << std::endl;
}

void Game_Cutting::UpdateGame(float delta)
{
    if (isFinished) return;

    // 홀딩 UI
    if (Input::GetKeyDown(spaceKey))
        spacebarHold->SetActive(true);
    else if(Input::GetKeyUp(spaceKey))
        spacebarHold->SetActive(false);
    
    // Space바 눌렀을때
    if (!isStopped && IsSpaceDownOnce())
    {
        isStopped = true;
        stopTimer = 0.0f;
        consumedThisStop = false;
    }

    // 판정 + 대기후 다시 움직이기
    if (isStopped)
    {
        if (!consumedThisStop)
        {
            consumedThisStop = true;
            EvaluateOnce();

            if (isFinished) return; // 5성공/3실패로 끝났으면 바로 종료
        }

        stopTimer += delta;
        if (stopTimer >= spaceStopTime)
        {
            isStopped = false;
            stopTimer = 0.0f;
            consumedThisStop = false;
        }

        return; // 멈춘 동안은 이동 x
    }

    // 스페이스 슬라이더 movement
    float x = GetUIPosX(knife);
    x += dir * spaceSpped * delta;

    if (x <= xMin)
    {
        x = xMin;
        dir = 1.0f;
    }
    else if (x >= xMax)
    {
        x = xMax;
        dir = -1.0f;
    }

    SetUIPosX(knife, x);
}

void Game_Cutting::EndGame()
{
    std::cout << "[MiniGame 1] : Game_Cutting End!" << std::endl;
}

void Game_Cutting::UpdateAnimation(float delta)
{
    if (animTimer <= 0.0f)
    {
        animTimer = changeAnimTime;
        currAnimIndex++;
        if (currAnimIndex >= currAnimPathes.size()) // 인덱스 초과 시 0으로
        {
            currAnimIndex = 0;
        }

        animImage->ChangeData(currAnimPathes[currAnimIndex]);
    }

    animTimer -= delta;
}

void Game_Cutting::SetAnimPathes()
{
    // 이미지 패스 생성
    currAnimPathes.resize(2);

    currAnimPathes[0] = "..\\Assets\\Resource\\UI\\MiniGame\\mini_01.png";
    currAnimPathes[1] = "..\\Assets\\Resource\\UI\\MiniGame\\mini_02.png";

    if(animImage)
        animImage->ChangeData(currAnimPathes[0]); // 첫번째로 초기화
}