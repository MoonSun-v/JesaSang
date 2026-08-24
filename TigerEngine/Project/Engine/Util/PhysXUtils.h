#pragma once
#include <PxPhysicsAPI.h>
#include <DirectXMath.h>

using namespace physx;
using namespace DirectX;


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
// Engine(cm) -> PhysX(m)
// PhysX(m)   -> Engine(cm)
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
inline XMVECTOR ToDXVec3(const PxVec3& v)
{
    return XMVectorSet(
        v.x * PHYSX_TO_WORLD, v.y * PHYSX_TO_WORLD, v.z * PHYSX_TO_WORLD,
        0.0f // w = 0
    );
}


// [ Quaternion ] ------------------------------------------
inline PxQuat ToPxQuat(const XMVECTOR& q)
{
    XMFLOAT4 f;
    XMStoreFloat4(&f, q);

    return PxQuat(f.x, f.y, f.z, f.w);
}

inline XMVECTOR ToDXQuat(const PxQuat& q)
{
    return XMVectorSet(q.x, q.y, q.z, q.w);
}

// PxQuat → XMFLOAT4
inline XMFLOAT4 ToDXQuatF4(const PxQuat& q)
{
    return XMFLOAT4(q.x, q.y, q.z, q.w);
}


// [ Scale ] ------------------------------------------
inline float PxToDX(float v)
{
    return v * PHYSX_TO_WORLD;
}


// 위치 -> ToDX(PxVec3)
// 크기 -> PxToDX(float)
// 회전 -> ToDXQuat / ToDXQuatF4