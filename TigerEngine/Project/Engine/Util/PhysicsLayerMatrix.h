#pragma once
#include "CollisionLayer.h"

class PhysicsLayerMatrix
{
public:
    static void Initialize();

    static bool CanCollide(CollisionLayer a, CollisionLayer b);

    static CollisionMask GetMask(CollisionLayer layer);

private:
    static void DisableCollision(CollisionLayer a, CollisionLayer b);

    static CollisionMask s_Matrix[32]; // Layer당 하나의 mask 저장 
};