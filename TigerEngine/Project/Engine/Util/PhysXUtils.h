#pragma once
#include <PxPhysicsAPI.h>
#include <DirectXMath.h>

using namespace physx;
using namespace DirectX;

// 축을 1:1로 대응하고 별도의 LH/RH 변환을 하지 않음. 주의. 


// ------------------------------------------
// [ Engine ↔ PhysX 변환 규칙 ]
// 
// Engine    : 1 unit = 1 cm
// PhysX     : 1 unit = 1 m
// ------------------------------------------

constexpr float WORLD_TO_PHYSX = 0.01f;   // cm -> m
constexpr float PHYSX_TO_WORLD = 100.0f;  // m  -> cm




// ------------------------------
// [ Position ]
// 
// Engine XMFLOAT3(cm) -> PhysX PxVec3(m)
// PhysX PxVec3(m)     -> Engine XMFLOAT3(cm)
// ------------------------------

inline PxVec3 ToPx(const XMFLOAT3& position)
{
    return PxVec3(
        position.x * WORLD_TO_PHYSX,
        position.y * WORLD_TO_PHYSX,
        position.z * WORLD_TO_PHYSX
    );
}

inline XMFLOAT3 ToDX(const PxVec3& position)
{
    return XMFLOAT3(
        position.x * PHYSX_TO_WORLD,
        position.y * PHYSX_TO_WORLD,
        position.z * PHYSX_TO_WORLD
    );
}


// PxVec3 -> XMVECTOR
inline XMVECTOR ToDXVec3(const PxVec3& value)
{
    return XMVectorSet(
        value.x * PHYSX_TO_WORLD,
        value.y * PHYSX_TO_WORLD,
        value.z * PHYSX_TO_WORLD,
        0.0f // w = 0
    );
}


// [ Direction / Normal ]
// 
// 방향 벡터와 표면 Normal은 단위를 가지지 않으므로 
// 단위 변환 없이 x, y, z 성분만 전달

inline PxVec3 ToPxDirection(const XMFLOAT3& direction)
{
    return PxVec3(
        direction.x,
        direction.y,
        direction.z
    );
}

inline XMFLOAT3 ToDXDirection(const PxVec3& direction)
{
    return XMFLOAT3(
        direction.x,
        direction.y,
        direction.z
    );
}

// ------------------------------
// [ Quaternion ] 
// 
// Quaternion의 x, y, z, w 성분 그대로 전달
// ------------------------------
inline PxQuat ToPxQuat(const XMVECTOR& quaternion)
{
    XMFLOAT4 value;
    XMStoreFloat4(&value, quaternion);

    return PxQuat(
        value.x,
        value.y,
        value.z,
        value.w
    );
}

inline XMVECTOR ToDXQuat(const PxQuat& quaternion)
{
    return XMVectorSet(
        quaternion.x,
        quaternion.y,
        quaternion.z,
        quaternion.w
    );
}

// PxQuat → XMFLOAT4
inline XMFLOAT4 ToDXQuatF4(const PxQuat& quaternion)
{
    return XMFLOAT4(
        quaternion.x,
        quaternion.y,
        quaternion.z,
        quaternion.w
    );
}



// ------------------------------
// [ Scale Length ] 
// 
// Collider의 반지름, 높이, 거리처럼 길이를 나타내는 단일 값 단위 변환
// ( Transform의 Scale은 단위가 없는 배율이므로 아래 함수를 사용해 변환하지 않음! ) 
// ------------------------------

// 길이 단위 변환
inline float ToPxLength(float centimeters)
{
    return centimeters * WORLD_TO_PHYSX;
}

inline float ToDXLength(float meters)
{
    return meters * PHYSX_TO_WORLD;
}

// TODO 제거 
inline float PxToDX(float v)
{
    return v * PHYSX_TO_WORLD;
}