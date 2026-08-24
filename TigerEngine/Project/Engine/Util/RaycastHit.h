#pragma once
#include <PxPhysicsAPI.h>
#include <DirectXMath.h>
#include <string>
#include "../Components/PhysicsComponent.h" 

class RaycastHit
{
public:
    // 감지된 엔진 Component
    PhysicsComponent* component = nullptr;

    // 엔진 좌표와 단위로 변환된 충돌 결과
    DirectX::XMFLOAT3 point = { 0.0f, 0.0f, 0.0f };  // cm
    DirectX::XMFLOAT3 normal = { 0.0f, 0.0f, 0.0f }; // 단위 방향 벡터
    float distance = 0.0f; // cm 

    // 디버그 / 확장용
    physx::PxShape* shape = nullptr;
    physx::PxRigidActor* actor = nullptr;
};