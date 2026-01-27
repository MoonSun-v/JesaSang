#pragma once
#include "pch.h"
#include "System/Singleton.h"
#include "../Components/Decal.h"

/*
    [ Decal System ]

    Decal Component를 Register하고 관리합니다.
    Decal Component는 따로 Update나 Render되지 않는 데이터 컴포넌트입니다.
    => Decal Pass에서 사용됩니다.
*/

class DecalSystem : public Singleton<DecalSystem>
{
private:
    std::vector<Decal*> comps{};

public:
    DecalSystem(token) {}
    ~DecalSystem() = default;

    void Register(Decal* comp);
    void UnRegister(Decal* comp);
    void Clear();

    std::vector<Decal*> GetComponents();
};

