#pragma once
#include <cstdint>
#include "../Base/Datas/EnumData.hpp"


constexpr uint32_t LayerCount = 32;
using CollisionMask = uint32_t;


// --------------------------------------------------
// Layer OR 연산자
// --------------------------------------------------
inline CollisionMask operator|(CollisionLayer a, CollisionLayer b)
{
    return (CollisionMask)a | (CollisionMask)b;
}

// 여러 개 이어서 쓸 수 있게
inline CollisionMask operator|(CollisionMask a, CollisionLayer b)
{
    return a | (CollisionMask)b;
}
