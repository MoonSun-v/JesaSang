#pragma once
#include "pch.h"
#include "System/Singleton.h"
#include "../Components/Light.h"

/*
    [ Light System ]

    Light Component를 Register하고 관리합니다.
    Light Component는 따로 Update나 Render되지 않는 데이터 컴포넌트입니다.

*/

class LightSystem : public Singleton<LightSystem>
{
private:
    std::vector<Light*> comps{};

public:
    LightSystem(token) {}
    ~LightSystem() = default;

    void Register(Light* comp);
    void Update();
    void UnRegister(Light* comp);
};