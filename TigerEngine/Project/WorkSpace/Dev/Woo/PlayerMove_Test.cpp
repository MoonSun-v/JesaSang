#include "PlayerMove_Test.h"

#include "Object/GameObject.h"
#include "System/InputSystem.h"
#include "EngineSystem/PhysicsSystem.h"
#include "Util/DebugDraw.h"
#include "Util/JsonHelper.h"

#include <directxtk/Keyboard.h>
#include <Util/PhysXUtils.h>

RTTR_REGISTRATION
{
    rttr::registration::class_<PlayerMove_Test>("PlayerMove_Test")
        .constructor<>()
        (rttr::policy::ctor::as_std_shared_ptr);
}

void PlayerMove_Test::OnInitialize()
{
    cctComp = GetOwner()->GetComponent<CharacterControllerComponent>();
}

void PlayerMove_Test::OnStart()
{
    if(!cctComp)
        OutputDebugStringW(L"OnStart의 cctComp가 null입니다. \n");
}

void PlayerMove_Test::OnUpdate(float delta)
{
    
}

void PlayerMove_Test::OnFixedUpdate(float dt)
{
    if (cctComp)
        Move(dt);
    else
        OutputDebugStringW(L"OnFixedUpdate의 cctComp가 null입니다. \n");
}

void PlayerMove_Test::OnCollisionEnter(PhysicsComponent* other)
{
    if (!other || !other->GetOwner()) return;

    auto name = this->GetOwner()->GetName();
    auto otherName = other->GetOwner()->GetName();

    cout << endl << "[CCT]" << name << " - OnCollisionEnter : " << otherName << endl << endl;

    std::wstring wName(name.begin(), name.end());
    std::wstring wOtherName(otherName.begin(), otherName.end());
    OutputDebugStringW((L"[CCT] " + wName + L" - OnCollisionEnter : " + wOtherName + L"\n").c_str());
}

nlohmann::json PlayerMove_Test::Serialize()
{
    return JsonHelper::MakeSaveData(this);
}

void PlayerMove_Test::Deserialize(nlohmann::json data)
{
    JsonHelper::SetDataFromJson(this, data);
}

void PlayerMove_Test::Move(float dt)
{
    // 걷기 
    Vector3 input(0, 0, 0);
    if (Input::GetKey(Keyboard::Up))    input.z += 1;
    if (Input::GetKey(Keyboard::Down))  input.z -= 1;
    if (Input::GetKey(Keyboard::Left))  input.x -= 1;
    if (Input::GetKey(Keyboard::Right)) input.x += 1;

    input.Normalize();

    // 점프
    bool spaceDown = Input::GetKey(Keyboard::Space) != 0;
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
