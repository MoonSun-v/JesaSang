#include "Components/CharacterControllerComponent.h"
#include "Object/GameObject.h"
#include "System/InputSystem.h"
#include "EngineSystem/PhysicsSystem.h"
#include "Util/DebugDraw.h"

#include "CCTTest.h"
#include <directxtk/Keyboard.h>
#include <Util/PhysXUtils.h>
#include <Components/FBXData.h>

using Key = DirectX::Keyboard::Keys;


template<typename T>
T Clamp(T v, T min, T max)
{
    return (v < min) ? min : (v > max) ? max : v;
}

RTTR_REGISTRATION
{
    rttr::registration::class_<CCTTest>("CCTTest")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

void CCTTest::OnInitialize() 
{
    cctComp = GetOwner()->GetComponent<CharacterControllerComponent>();
    animController = GetOwner()->GetComponent<AnimationController>();
}

void CCTTest::OnStart() 
{
    if (cctComp != nullptr)
    {

    }
    else
    {
        OutputDebugStringW(L"[CCTTest] OnStart의 cctComp가 null입니다. \n");
    }


    // [ 애니메이션 FSM 등록 ] 
    if (animController != nullptr)
    {
        auto fbx = GetOwner()->GetComponent<FBXData>();

        // 애니메이션 파일 로드 (FBXData, 경로, 애니메이션 이름, loop여부)
        FBXResourceManager::Instance().LoadAnimationByPath(fbx->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\Test_Idle.fbx", "Idle");
        FBXResourceManager::Instance().LoadAnimationByPath(fbx->GetFBXInfo(), "..\\Assets\\Resource\\Animation\\Test_Walk.fbx", "Walk");

        auto idleClip = animController->FindClip("Idle");
        auto walkClip = animController->FindClip("Walk");

        if (!idleClip || !walkClip)
        {
            OutputDebugStringW(L"[CCTTest] Clip not found! 이름 확인 필요\n");
            return;
        }

        // 상태 등록
        animController->AddState(std::make_unique<AnimationState>("Idle", idleClip, animController));
        animController->AddState(std::make_unique<AnimationState>("Walk", walkClip, animController));

        // 시작 상태
        animController->ChangeState("Idle");

        elapsedTime = 0.0f;
    }
    else
    {
        OutputDebugStringW(L"[CCTTest] OnStart의 animController가 null입니다. \n");
    }
    
}

// 3초 후에 Walk 상태로 전환 
void CCTTest::OnUpdate(float delta)
{
    if (!animController || hasRunStateChanged) return;

    elapsedTime += delta;

    if (elapsedTime >= 3.0f)
    {
        animController->ChangeState("Walk", 0.2f); // 상태 전환 
        hasRunStateChanged = true;
    }
}

void CCTTest::OnFixedUpdate(float dt)
{
    if (!cctComp)
    {
        OutputDebugStringW(L"[CCTTest] OnFixedUpdate의 cctComp가 null입니다. \n");
        return;
    }

    CCTMoveExample(dt);  // 캐릭터 이동 예시 코드 
    // RaycastExample(); // 레이캐스트  예시 코드 
}


// [ 충돌 이벤트 함수 ] 
void CCTTest::OnCollisionEnter(PhysicsComponent* other)
{
    if (!other || !other->GetOwner()) return;

    auto name = this->GetOwner()->GetName();
    auto otherName = other->GetOwner()->GetName();

    cout << endl << "[CCT]" << name << " - OnCollisionEnter : " << otherName << endl << endl;

    std::wstring wName(name.begin(), name.end());
    std::wstring wOtherName(otherName.begin(), otherName.end());
    OutputDebugStringW((L"[CCT] "+ wName + L" - OnCollisionEnter : " + wOtherName + L"\n").c_str());
}


// [ CCT를 소유한 캐릭터의 움직임 구현 코드 예시 ]
// - 걷기, 점프 구현 예시 
void CCTTest::CCTMoveExample(float dt)
{
    // 걷기 
    Vector3 input(0, 0, 0);
    if (Input::GetKey(Key::Up))    input.z += 1;
    if (Input::GetKey(Key::Down))  input.z -= 1;
    if (Input::GetKey(Key::Left))  input.x -= 1;
    if (Input::GetKey(Key::Right)) input.x += 1;

    input.Normalize();

    // 점프
    bool spaceDown = Input::GetKey(Key::Space) != 0;
    if (spaceDown && !m_SpacePrev)
    {
        cctComp->Jump();
    }
    m_SpacePrev = spaceDown;

    // yaw 기준 이동 벡터 계산
    float yaw = GetOwner()->GetTransform()->GetYaw();

    Vector3 forward = { sinf(yaw), 0, cosf(yaw) };
    Vector3 right = { cosf(yaw), 0,-sinf(yaw) };

    Vector3 moveDir = forward * input.z + right * input.x;
    if (moveDir.LengthSquared() > 0)
        moveDir.Normalize();


    // ----------------------------
    // MoveCharacter는 입력이 없더라도 무.족.권. 항상 호출되어야 함!
    // CCT의 물리 연산을 담당 
    // 즉, 이 코드는 무슨일이 있어도 실행 되고 있어야함. 지우지 마세요 
    // ----------------------------
    cctComp->MoveCharacter(moveDir, dt);
}


// [ CCT를 소유한 객체의 레이캐스트 예제 ]
// - 아래 방향으로 레이캐스트 발사
// - 레이캐스트가 충돌한 객체의 이름 디버그 출력 
void CCTTest::RaycastExample()
{
    Vector3 offset(0, 60.0f, 0);
    PxVec3 originPx = ToPx(GetOwner()->GetTransform()->GetPosition() + offset);

    // 아래 방향
    PxVec3 downPx(0, -1, 0);

    QueryTriggerInteraction triggerInteraction = QueryTriggerInteraction::Ignore;

    std::vector<RaycastHit> hits;
    bool bHit = PhysicsSystem::Instance().Raycast(
        originPx,
        downPx,
        200.0f,
        hits,
        CollisionLayer::Player,
        triggerInteraction,
        false   // 첫 번째만
    );

    if (!bHit || hits.empty())
    {
        OutputDebugStringW(L"[CCTTest] Raycast miss\n");
        return;
    }

    // 첫 번째 히트
    RaycastHit& hit = hits[0];
    PxVec3 endPx = hit.point;

    // 디버그 레이 그리기 (빨간색)
    DebugDraw::DrawRayDebug(
        nullptr, // batch 안 쓰면 내부에서 처리하거나 전역 batch 사용
        ToDXVec3(originPx),
        ToDXVec3(endPx - originPx),
        XMVectorSet(1, 0, 0, 1),
        false
    );

    // 이름 출력
    std::wstring hitName = L"Unknown";
    if (hit.component && hit.component->GetOwner())
    {
        std::string name = hit.component->GetOwner()->GetName();
        hitName = std::wstring(name.begin(), name.end());
    }

    wchar_t buf[256];
    swprintf(buf, 256, L"[CCTTest] Hit: %s at distance %.2f\n", hitName.c_str(), hit.distance);
    OutputDebugStringW(buf);
}
