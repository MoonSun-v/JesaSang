#pragma once
#include "../pch.h"
#include "../System/Singleton.h"
#include "../System/SceneSystem.h"

class Camera;
class GameObject;

/// @brief 카메라 컴포넌트를 관리하는 시스템 클래스
/// @date 26.01.09
/// @details 씬의 카메라는 해당 클래스에 의해 관리됩니다.
class CameraSystem : public Singleton<CameraSystem>
{
public:
    CameraSystem(token) {};
    ~CameraSystem() = default;

    void SetScreenSize(int width, int height);
    Vector2 GetScreenSize() { return Vector2(screenWidth, screenHeight); }

    void FreeCameraUpdate(float delta);

    /// @brief 프로그램 초기화 시 실행되는 프리캠 생성 함수
    /// @param width 화면 넓이
    /// @param height 화면 높이
    Handle CreateFreeCamera(int width, int height, Scene* currScene);
    Camera* GetFreeCamera();

    void Register(Camera* cam);
    void RemoveCamera(Camera* cam);
    Camera* GetCameraByIndex(int index);
    std::vector<Camera*> GetAllCamera() { return registered; }

    int SetCurrCamera(int index);

    Camera* GetCurrCamera() 
    { 
        return registered[currCameraIndex];
    }

    void NextCamera() 
    { 
        currCameraIndex++;
        if (currCameraIndex >= registered.size())
        {
            currCameraIndex = 0;
        }
    }
    void PrevCamera() 
    {
        currCameraIndex--;
        if (currCameraIndex < 0)
        {
            currCameraIndex = registered.size() - 1;
        }
    }

    void Clear();

private:
    GameObject* freeCamObj{};
    Camera* freeCamera{};    // 프로그램 시작 시 등록
    int currCameraIndex = 0; // 현재 사용하는 카메라 인덱스

    std::vector<Camera*> registered{};              // 등록된 카메라 배열
    std::map<int, Camera*> mappedRegistered{};      // 찾기 위한 매핑 배열

    int screenWidth{};
    int screenHeight{};
};