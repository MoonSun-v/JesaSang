#include "Components/CharacterControllerComponent.h"
#include "Object/GameObject.h"
#include "System/InputSystem.h"

#include "CCTTest.h"
#include <directxtk/Keyboard.h>

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

void CCTTest::OnInitialize() // editor
{
}

void CCTTest::OnStart() // ?? 아직 사용하지X 
{

}

void CCTTest::OnUpdate(float delta)
{
    // OnStart() 대용 
    if (!isApply)
    {
        cctComp = GetOwner()->GetComponent<CharacterControllerComponent>();
        if (cctComp != nullptr)
        {
            cctComp->CreateCharacterCollider(30.0f, 120.0f, { 0,50,0 });
            // cctComp->SetLayer(CollisionLayer::IgnoreTest); // 충돌 레이어 테스트 
        }

        isApply = true;
    }
}

void CCTTest::OnFixedUpdate(float dt)
{
    // OutputDebugString((L"[CCTTest] : OnFixedUpdate 실행 \n\n"));

    if (!cctComp)
    {
        OutputDebugStringW(L"[CCTTest] OnFixedUpdate의 cctComp가 null입니다. \n");
        return;
    }

    Vector3 input(0, 0, 0);
    if (Input::GetKey(Key::Up))    input.z += 1;
    if (Input::GetKey(Key::Down))  input.z -= 1;
    if (Input::GetKey(Key::Left))  input.x -= 1;
    if (Input::GetKey(Key::Right)) input.x += 1;

    //if (GetAsyncKeyState(VK_UP) & 0x8000)    input.z += 1;
    //if (GetAsyncKeyState(VK_DOWN) & 0x8000)  input.z -= 1;
    //if (GetAsyncKeyState(VK_LEFT) & 0x8000)  input.x -= 1;
    //if (GetAsyncKeyState(VK_RIGHT) & 0x8000) input.x += 1;

   input.Normalize();


   // 점프 입력
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
    // MoveCharacter 는 입력이 없더라도 항상 호출되어야 함! 
    // ----------------------------
    cctComp->MoveCharacter(moveDir, dt);
}