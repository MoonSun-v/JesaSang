#include "Components/CharacterControllerComponent.h"
#include "Object/GameObject.h"

#include "CCTTest.h"

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
        auto cct = GetOwner()->GetComponent<CharacterControllerComponent>();
        if (cct != nullptr)
        {
            cct->CreateCharacterCollider(30.0f, 120.0f, { 0,50,0 });
            // cct->SetLayer(CollisionLayer::IgnoreTest); // 충돌 레이어 테스트 
        }

        isApply = true;
    }
}

void CCTTest::OnFixedUpdate(float dt)
{
    // OutputDebugString((L"[CCTTest] : OnFixedUpdate 실행 \n\n"));
}