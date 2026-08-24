#include "PhysicsLayerMatrix.h"

namespace
{
    int LayerToIndex(CollisionLayer layer)
    {
        uint32_t value = static_cast<uint32_t>(layer);
        int index = 0;

        // value가 1이 될 때까지 오른쪽으로 시프트
        while (value > 1)
        {
            value >>= 1;
            ++index;
        }

        return index;
    }
}

// Layer마다 충돌 가능한 다른 Layer들의 Bit Mask를 저장한다.
CollisionMask PhysicsLayerMatrix::s_Matrix[LayerCount];


void PhysicsLayerMatrix::Initialize()
{
    // 기본값 : 모든 레이어는 서로 충돌
    for (int i = 0; i < LayerCount; ++i)
    {
        s_Matrix[i] = 0xFFFFFFFF;
    }

    // -------------------------------------
    // 여기서 [ 레이어 조합 비활성화 ] 설정 하세요
    // -------------------------------------

    // Plyaer <-> Player : 서로 충돌 안함
    DisableCollision(CollisionLayer::Player, CollisionLayer::Player);

    // Enemy <-> Enemy : 서로 충돌 안함
    DisableCollision(CollisionLayer::Enemy, CollisionLayer::Enemy);

    // Ground <-> Vision : Vision은 Ground를 감지하지 않음
    DisableCollision(CollisionLayer::Ground, CollisionLayer::Vision);
}

CollisionMask PhysicsLayerMatrix::GetMask(CollisionLayer layer)
{
    return s_Matrix[LayerToIndex(layer)];
}


// 두 Layer가 서로를 허용하는 경우에만 상호작용 
// - Matrix 설정 실수로 한쪽 Mask만 활성화된 경우에도 결과가 인자 순서에 따라 달라지지 않도록 양방향을 검사.
bool PhysicsLayerMatrix::CanCollide(CollisionLayer a, CollisionLayer b)
{
    const bool aAllowsB = (GetMask(a) & static_cast<CollisionMask>(b)) != 0;

    const bool bAllowsA = (GetMask(b) & static_cast<CollisionMask>(a)) != 0;

    return aAllowsB && bAllowsA;
}

void PhysicsLayerMatrix::DisableCollision(CollisionLayer a, CollisionLayer b)
{
    // A가 허용하는 Layer 목록에서 B를 제거한다.
    s_Matrix[LayerToIndex(a)] &= ~static_cast<CollisionMask>(b);

    // B가 허용하는 Layer 목록에서 A를 제거한다.
    s_Matrix[LayerToIndex(b)] &= ~static_cast<CollisionMask>(a);
}