#pragma once
#include "Components/ScriptComponent.h"
#include <directxtk/SimpleMath.h>

class Transform;

/*
     [ Camera Controller Script Component ]

     1) Target Tracking (deadzone + clamp follow) : XZ 이동
     2) Target Look Focus (range 안에서만 Yaw 회전)
     3) 시점 변경 연출 (Quarter <-> Top)
*/

class CameraController : public ScriptComponent
{
    RTTR_ENABLE(ScriptComponent)

public:
    // 시점 변경 연출 (일단 Top으로 구분)
    enum class ViewMode
    {
        Quarter, Top
    };

private:
    // --- [ Component ] -----------------------
    Transform* transform = nullptr;
    Transform* targetTr = nullptr;


    // --- [ Target Tracking ] -----------------
    Vector3 quarterOffset = { -250.0f, 250.0f, -250.0f }; 
    Vector3 topOffset     = { 0.0f,   450.0f,  0.0f };        

    float followDeadZone = 60.0f;    // deadzone (넘어가면 trackings)
    float followSpeed = 12.0f;       // tracking speed

    // --- [ Look Focus ] ----------------------
    bool  enableLookFocus = true;
    float focusRange = 220.0f;     // focus range (이내일때만 yaw 회전)
    float focusYawSpeed = 10.0f;   // yaw lerp speed

    // --- [ View Transition ] -----------------
    ViewMode currentMode = ViewMode::Quarter;
    ViewMode targetMode = ViewMode::Quarter;

    bool  isTransitioning = false;
    float transitionDuration = 0.35f;
    float transitionT = 0.0f;

    Vector3 quarterEuler = { -0.55f, 0.78f, 0.0f };
    Vector3 topEuler = { -1.55f, 0.0f,  0.0f };    


public:
    // Component process
    void OnInitialize() override;
    void OnStart() override;
    void OnLateUpdate(float delta) override;
    void OnDestory() override;

    // Json
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);

public:
    // Funcs..
    static float Clamp01(float v);
    static float SmoothStep01(float t);
    static float WrapAngleRad(float a);
    static float Lerp(float a, float b, float t);
    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t);

    Vector3 GetModeOffset(ViewMode mode) const;
    Vector3 GetModeEuler(ViewMode mode) const;

    void UpdateViewTransition(float delta);
    void TargetTracking(float delta);
    void TargetLookFocus(float delta);

public:
    // 외부 call func
    void SetViewMode(ViewMode mode);
};

