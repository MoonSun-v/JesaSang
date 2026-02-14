#include "Game_Assembling.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

#include "EngineSystem/SceneSystem.h"
#include "System/InputSystem.h"
#include "Object/GameObject.h"
#include "Components/UI/Image.h"
#include "../../../Ho/Sound/SoundManager.h"

using std::cout;
using std::endl;


void Game_Assembling::SetImage(Image* image, string& path)
{
    if (!image) return;
    image->ChangeData(path);
}

void Game_Assembling::BuildDirImageTable()
{
    imgLeft = DirImages{ &leftDefault,  &leftGood,  &leftFail };
    imgRight = DirImages{ &rightDefault, &rightGood, &rightFail };
    imgUp = DirImages{ &upDefault,    &upGood,    &upFail };
    imgDown = DirImages{ &downDefault,  &downGood,  &downFail };
}

const Game_Assembling::DirImages& Game_Assembling::GetDirImages(ArrowDir dir) const
{
    switch (dir)
    {
    case ArrowDir::Left:  return imgLeft;
    case ArrowDir::Right: return imgRight;
    case ArrowDir::Up:    return imgUp;
    default:              return imgDown;
    }
}

void Game_Assembling::GeneratePattern()
{
    for (int i = 0; i < 8; ++i)
    {
        const int r = std::rand() % 4;
        pattern[i] = static_cast<ArrowDir>(r);
    }
}

void Game_Assembling::ApplyDefaultImages()
{
    for (int i = 0; i < 8; ++i)
    {
        const DirImages& imgs = GetDirImages(pattern[i]);
        SetImage(uiKeys[i], *imgs.def);
    }
}

void Game_Assembling::StartNewTurn()
{
    curIndex = 0;
    curTurnFailCount = 0;
    keyTimer = 0.0f;

    GeneratePattern();
    ApplyDefaultImages();
}

bool Game_Assembling::TryGetArrowDown(ArrowDir& outDir) const
{
    // "한 번 눌림"만 체크 (GetKeyDown)
    if (InputSystem::Instance().GetKeyDown(Keyboard::Keys::Left))
    {
        outDir = ArrowDir::Left;
        return true;
    }
    if (InputSystem::Instance().GetKeyDown(Keyboard::Keys::Right))
    {
        outDir = ArrowDir::Right;
        return true;
    }
    if (InputSystem::Instance().GetKeyDown(Keyboard::Keys::Up))
    {
        outDir = ArrowDir::Up;
        return true;
    }
    if (InputSystem::Instance().GetKeyDown(Keyboard::Keys::Down))
    {
        outDir = ArrowDir::Down;
        return true;
    }
    return false;
}

void Game_Assembling::MarkCurrentSlotGood()
{
    const DirImages& imgs = GetDirImages(pattern[curIndex]);
    SetImage(uiKeys[curIndex], *imgs.good);
}

void Game_Assembling::MarkCurrentSlotFail()
{
    const DirImages& imgs = GetDirImages(pattern[curIndex]);
    SetImage(uiKeys[curIndex], *imgs.fail);
}

void Game_Assembling::AdvanceIndex()
{
    curIndex++;
    keyTimer = 0.0f;
}

void Game_Assembling::ResolveHit(bool correct)
{
    if (correct)
    {
        MarkCurrentSlotGood();
        SoundManager::Instance()->PlaySFX(SFXType::Minigame_Success_Sound); // 중간 성공
    }
    else
    {
        MarkCurrentSlotFail();
        SoundManager::Instance()->PlaySFX(SFXType::Minigame_Wrong_Sound);   // 중간 실패
        curTurnFailCount++;
    }

    AdvanceIndex();
}

void Game_Assembling::FinishTurnIfNeeded()
{
    // 턴 중 2번 틀리면 즉시 실패
    if (curTurnFailCount >= 2)
    {
        isFinished = true;
        isSuccess = false;
        SoundManager::Instance()->PlaySFX(SFXType::Minigame_Fail_Sound);
        return;
    }

    // 8개를 다 처리했으면 턴 성공
    if (curIndex >= 8)
    {
        curTurnSuccessCount++;

        if (curTurnSuccessCount >= 3)
        {
            isFinished = true;
            isSuccess = true;
            SoundManager::Instance()->PlaySFX(SFXType::Minigame_Clear_Sound);
            return;
        }

        // 다음 턴 시작
        StartNewTurn();
    }
}

void Game_Assembling::StartGame()
{
    key1 = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("3_Image_key1")->GetComponent<Image>();
    key2 = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("3_Image_key2")->GetComponent<Image>();
    key3 = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("3_Image_key3")->GetComponent<Image>();
    key4 = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("3_Image_key4")->GetComponent<Image>();
    key5 = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("3_Image_key5")->GetComponent<Image>();
    key6 = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("3_Image_key6")->GetComponent<Image>();
    key7 = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("3_Image_key7")->GetComponent<Image>();
    key8 = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("3_Image_key8")->GetComponent<Image>();

    animImage = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("3_Image_CutAnimation")->GetComponent<Image>();


    if (!key1 || !key2 || !key3 || !key4 || !key5 || !key6 || !key7 || !key8)
    {
        cout << "[MiniGame 3] Missing ui objects!" << endl;
        return;
    }

    // 배열로 묶기
    uiKeys = { key1, key2, key3, key4, key5, key6, key7, key8 };

    // seed
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // init
    isFinished = false;
    isSuccess = false;

    curTurnSuccessCount = 0;
    curTurnFailCount = 0;
    curIndex = 0;
    keyTimer = 0.0f;

    BuildDirImageTable();
    StartNewTurn();
    SetAnimPathes();
    cout << "[Mini Game 3] : Game_Assembling Start!" << endl;
}

void Game_Assembling::UpdateGame(float delta)
{
    if (isFinished) return;

    // 현재 슬롯 시간 제한 체크
    keyTimer += delta;

    // 입력 받기
    ArrowDir inputDir;
    const bool pressed = TryGetArrowDown(inputDir);

    if (pressed)
    {
        const ArrowDir expected = pattern[curIndex];
        const bool correct = (inputDir == expected);
        ResolveHit(correct);
        FinishTurnIfNeeded();
        return;
    }

    // 시간 초과면 자동 실패 처리
    if (keyTimer >= keyTimeLimit)
    {
        ResolveHit(false);
        FinishTurnIfNeeded();
        return;
    }
}

void Game_Assembling::EndGame()
{
    cout << "[Mini Game] : Game_Assembling End!" << endl;
}

void Game_Assembling::UpdateAnimation(float delta)
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

void Game_Assembling::SetAnimPathes()
{
    // 이미지 패스 생성
    currAnimPathes.resize(2);

    currAnimPathes[0] = "..\\Assets\\Resource\\UI\\MiniGame\\mini_01.png";
    currAnimPathes[1] = "..\\Assets\\Resource\\UI\\MiniGame\\mini_02.png";

    if (animImage)
        animImage->ChangeData(currAnimPathes[0]); // 첫번째로 초기화
}