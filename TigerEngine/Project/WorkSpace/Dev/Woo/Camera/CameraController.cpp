#include "CameraController.h"
#include "Util/JsonHelper.h"
#include "Util/ComponentAutoRegister.h"

#include "Object/GameObject.h"
#include "EngineSystem/SceneSystem.h"


REGISTER_COMPONENT(CameraController)

RTTR_REGISTRATION
{
    rttr::registration::class_<CameraController>("CameraController")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

void CameraController::OnInitialize()
{
}

void CameraController::OnStart()
{
    // get components
    transform = GetOwner()->GetComponent<Transform>();
    targetTr = SceneSystem::Instance().GetCurrentScene()->GetGameObjectByName("Player")->GetComponent<Transform>();
}

void CameraController::OnLateUpdate(float delta)
{
    if (!transform || !targetTr) return;

    UpdateViewTransition(delta);
    TargetTracking(delta);
    TargetLookFocus(delta);
}

void CameraController::OnDestory()
{
}

nlohmann::json CameraController::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void CameraController::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

float CameraController::Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

float CameraController::SmoothStep01(float t)
{
    // smoothstep: t*t*(3-2t)
    t = Clamp01(t);
    return t * t * (3.0f - 2.0f * t);
}

float CameraController::WrapAngleRad(float a)
{
    while (a > DirectX::XM_PI)  a -= DirectX::XM_2PI;
    while (a < -DirectX::XM_PI) a += DirectX::XM_2PI;
    return a;
}

float CameraController::Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

Vector3 CameraController::Lerp(const Vector3& a, const Vector3& b, float t)
{
    return a + (b - a) * t;
}

Vector3 CameraController::GetModeOffset(ViewMode mode) const
{
    return (mode == ViewMode::Quarter) ? quarterOffset : topOffset;
}

Vector3 CameraController::GetModeEuler(ViewMode mode) const
{
    return (mode == ViewMode::Quarter) ? quarterEuler : topEuler;
}

void CameraController::UpdateViewTransition(float delta)
{
    if (!isTransitioning) return;

    transitionT += delta;
    float t = (transitionDuration <= 0.0001f) ? 1.0f : (transitionT / transitionDuration);
    t = SmoothStep01(t);

    // 위치: 플레이어 기준 offset 보간
    Vector3 fromOffset = GetModeOffset(currentMode);
    Vector3 toOffset = GetModeOffset(targetMode);
    Vector3 blendedOffset = Lerp(fromOffset, toOffset, t);

    Vector3 desiredPos = targetTr->GetWorldPosition() + blendedOffset;

    // 회전: euler 보간(여기서는 pitch/yaw만 쓴다고 가정)
    Vector3 fromE = GetModeEuler(currentMode);
    Vector3 toE = GetModeEuler(targetMode);

    // yaw는 wrap 보간(경계에서 빙글 방지)
    float yawA = fromE.y;
    float yawB = toE.y;
    float dy = WrapAngleRad(yawB - yawA);
    float blendedYaw = yawA + dy * t;

    float blendedPitch = Lerp(fromE.x, toE.x, t);

    transform->SetPosition(desiredPos);
    transform->SetEuler(Vector3(blendedPitch, blendedYaw, 0.0f));

    if (transitionT >= transitionDuration)
    {
        // 종료
        isTransitioning = false;
        currentMode = targetMode;

        // 딱 맞춰 스냅
        transform->SetPosition(targetTr->GetWorldPosition() + GetModeOffset(currentMode));
        transform->SetEuler(GetModeEuler(currentMode));
    }
}

// --------------------
// 1) Target Tracking (deadzone + clamp)
// --------------------
void CameraController::TargetTracking(float delta)
{
    // “카메라가 있어야 할 기준점” = playerPos + modeOffset
    Vector3 desired = targetTr->GetWorldPosition() + GetModeOffset(isTransitioning ? targetMode : currentMode);

    Vector3 camPos = transform->GetWorldPosition();

    // XZ만 비교
    Vector3 diff = desired - camPos;
    diff.y = 0.0f;

    float dist = diff.Length();

    // deadzone 안이면 이동 없음(카메라 흔들림 제거)
    if (dist <= followDeadZone)
        return;

    // deadzone을 넘은 만큼만 따라간다:
    // camPos를 desired 방향으로 (dist - deadzone)만큼 이동해서 “경계선까지” 복귀
    Vector3 dir = diff;
    if (dir.LengthSquared() > 0.0001f)
        dir.Normalize();

    Vector3 clampedTargetPos = camPos + dir * (dist - followDeadZone);

    // 부드러운 보간 (프레임 독립)
    float t = 1.0f - expf(-followSpeed * delta);
    Vector3 newPos = Lerp(camPos, Vector3(clampedTargetPos.x, camPos.y, clampedTargetPos.z), t);

    // Y는 모드 offset의 높이로 유지하고 싶으면 아래처럼 고정 가능:
    // newPos.y = desired.y;
    // (현재는 camPos.y 유지)

    transform->SetPosition(newPos);
}

// --------------------
// 2) Target Look Focus (range 안에서만 yaw 업데이트)
// --------------------
void CameraController::TargetLookFocus(float delta)
{
    if (!enableLookFocus) return;
    if (!targetTr) return;

    Vector3 targetPos = targetTr->GetWorldPosition();
    Vector3 center = targetTr->GetWorldPosition() + GetModeOffset(isTransitioning ? targetMode : currentMode);
    Vector3 to = targetPos - center;
    to.y = 0;

    if (to.LengthSquared() <= 0.0001f)
        return;

    float dist = to.Length();
    if (dist > focusRange)
        return; // range 밖이면 look focus 하지 않음

    // 목표 yaw (카메라가 target을 바라보는 방향)
    float targetYaw = atan2f(to.x, to.z);

    // 현재 euler 얻어서 yaw만 부드럽게 변경
    Vector3 e = transform->GetEuler();
    float currentYaw = e.y;

    float dy = WrapAngleRad(targetYaw - currentYaw);
    float t = 1.0f - expf(-focusYawSpeed * delta);

    float newYaw = currentYaw + dy * t;

    // pitch/roll은 유지 (모드 연출용 pitch 유지)
    transform->SetEuler(Vector3(e.x, newYaw, 0.0f));
}


void CameraController::SetViewMode(ViewMode mode)
{
    if (mode == targetMode) return;

    targetMode = mode;
    isTransitioning = true;
    transitionT = 0.0f;
}
