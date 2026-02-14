#include "Game_FireControl.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>   // rand
#include <ctime>     // time

#include "EngineSystem/SceneSystem.h"
#include "System/InputSystem.h"
#include "Object/GameObject.h"
#include "Components/RectTransform.h"
#include "Components/UI/Image.h"
#include "../../../Ho/Sound/SoundManager.h"

using std::cout;
using std::endl;


float Game_FireControl::GetUIPosX(RectTransform* rt) const
{
    return rt->GetPos().x;
}

void Game_FireControl::SetUIPosX(RectTransform* rt, float x)
{
    Vector3 pos = rt->GetPos();
    pos.x = x;
    rt->SetPos(pos);
}

float Game_FireControl::GetUIWidth(RectTransform* rt) const
{
    return rt->GetSize().x;
}

float Game_FireControl::Clamp(float v, float mn, float mx) const
{
    return std::max(mn, std::min(mx, v));
}

float Game_FireControl::RandRange(float a, float b) const
{
    const float t = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    return a + (b - a) * t;
}

bool Game_FireControl::IsSpaceHold() const
{
    return InputSystem::Instance().GetKey(spaceKey);
}

void Game_FireControl::PickNextFoodTarget()
{
    const float curX = GetUIPosX(foodPoint);

    float t = RandRange(xMin, xMax);
    int safety = 0;
    while (std::abs(t - curX) < 60.0f && safety < 10)
    {
        t = RandRange(xMin, xMax);
        safety++;
    }

    foodTargetX = t;
    foodSpeed = RandRange(foodSpeedMin, foodSpeedMax);

    // 이동 후 잠깐 멈춤
    foodStopDuration = RandRange(foodStopMin, foodStopMax);
}

void Game_FireControl::UpdateFood(float delta)
{
    if (foodStopped)
    {
        foodStopTimer += delta;
        if (foodStopTimer >= foodStopDuration)
        {
            foodStopped = false;
            foodStopTimer = 0.0f;
            PickNextFoodTarget();
        }
        return;
    }

    float x = GetUIPosX(foodPoint);

    const float dir = (foodTargetX >= x) ? 1.0f : -1.0f;
    x += dir * foodSpeed * delta;

    // 목표 근처 도착 처리
    if (std::abs(foodTargetX - x) <= foodTargetEps ||
        (dir > 0.0f && x >= foodTargetX) ||
        (dir < 0.0f && x <= foodTargetX))
    {
        x = foodTargetX;
        SetUIPosX(foodPoint, x);

        foodStopped = true;
        foodStopTimer = 0.0f;
        return;
    }

    x = Clamp(x, xMin, xMax);
    SetUIPosX(foodPoint, x);
}

void Game_FireControl::UpdateBar(float delta)
{
    float x = GetUIPosX(spacbar);

    // 스듀 느낌 : 홀딩하면 +가속(오른쪽/위로), 아니면 -가속(왼쪽/아래로)
    if (IsSpaceHold())
    {
        barVel += barAccelUp * delta;
        barVel = Clamp(barVel, -barMaxVelDown, barMaxVelUp);
    }
    else
    {
        barVel -= barAccelDown * delta;
        barVel = Clamp(barVel, -barMaxVelDown, barMaxVelUp);
    }

    // 약간 감쇠(너무 튀지 않게)
    barVel *= (1.0f - Clamp(barFriction * delta, 0.0f, 0.35f));

    x += barVel * delta;

    // 바운더리 충돌 처리
    if (x <= xMin)
    {
        x = xMin;
        barVel = 0.0f;
    }
    else if (x >= xMax)
    {
        x = xMax;
        barVel = 0.0f;
    }

    SetUIPosX(spacbar, x);
}

bool Game_FireControl::IsFoodInsideBar() const
{
    const float foodX = GetUIPosX(foodPoint);
    const float foodW = std::max(1.0f, GetUIWidth(foodPoint));

    const float barX = GetUIPosX(spacbar);
    const float barW = std::max(1.0f, GetUIWidth(spacbar));

    const float barLeft = barX - barW * 0.5f;
    const float barRight = barX + barW * 0.5f;

    // 스듀처럼 food 크기도 반영gkdu food의 좌/우가 바 안에 들어오는지 체크
    const float foodLeft = foodX - foodW * 0.5f;
    const float foodRight = foodX + foodW * 0.5f;

    // food가 bar 안에 들어온다 기준 (겹침(Overlap)으로 판단) 
    const bool overlap = !(foodRight < barLeft || foodLeft > barRight);
    return overlap;
}

void Game_FireControl::UpdateGauge(float delta)
{
    const bool inside = IsFoodInsideBar();

    if (inside)
        gaugeValue += gaugeUpPerSec * delta;
    else
        gaugeValue -= gaugeDownPerSec * delta;

    gaugeValue = Clamp(gaugeValue, 0.0f, 1.0f);

    gauge->SetFillAmount(gaugeValue);

    // 성공/실패
    if (gaugeValue >= 1.0f)
    {
        isFinished = true;
        isSuccess = true;
        SoundManager::Instance()->PlaySFX(SFXType::Minigame_Clear_Sound);
    }
    else if (gaugeValue <= 0.0f)
    {
        isFinished = true;
        isSuccess = false;
        SoundManager::Instance()->PlaySFX(SFXType::Minigame_Fail_Sound);
    }
}

void Game_FireControl::StartGame()
{
    foodPoint = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("2_Image_FoodPoint")->GetComponent<RectTransform>();
    spacbar = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("2_Image_SpaceBar")->GetComponent<RectTransform>();
    gauge = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("2_Image_Gauge")->GetComponent<Image>();
    animImage = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("2_Image_CutAnimation")->GetComponent<Image>();
    
    spacebarHold = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Image_SpaceBarHold");

    if (!foodPoint || !spacbar || !gauge || !spacebarHold)
    {
        cout << "[MiniGame 2] Missing ui objects!" << endl;
        return;
    }

    // time
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // init
    isFinished = false;
    isSuccess = false;

    // start positions
    SetUIPosX(foodPoint, (xMin + xMax) * 0.5f);
    SetUIPosX(spacbar, (xMin + xMax) * 0.5f);

    // gauge init
    gaugeValue = 0.5f;
    gauge->SetFillAmount(gaugeValue);

    // food init
    foodStopped = false;
    foodStopTimer = 0.0f;
    PickNextFoodTarget();
    SetAnimPathes();

    // bar init
    barVel = 0.0f;

    // hold ui off
    spacebarHold->SetActive(false);

    cout << "[Mini Game 2] : Game_FireControl Start!" << endl;
}

void Game_FireControl::UpdateGame(float delta)
{
    if (isFinished) return;

    // 홀딩 UI
    if (Input::GetKeyDown(spaceKey))
        spacebarHold->SetActive(true);
    else if (Input::GetKeyUp(spaceKey))
        spacebarHold->SetActive(false);

    UpdateFood(delta);
    UpdateBar(delta);
    UpdateGauge(delta);
}

void Game_FireControl::EndGame()
{
    std::cout << "[Mini Game 2] : Game_FireControl End!" << std::endl;

}

void Game_FireControl::UpdateAnimation(float delta)
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

void Game_FireControl::SetAnimPathes()
{
    // 이미지 패스 생성
    currAnimPathes.resize(2);

    currAnimPathes[0] = "..\\Assets\\Resource\\UI\\MiniGame\\mini_01.png";
    currAnimPathes[1] = "..\\Assets\\Resource\\UI\\MiniGame\\mini_02.png";

    if (animImage)
        animImage->ChangeData(currAnimPathes[0]); // 첫번째로 초기화
}