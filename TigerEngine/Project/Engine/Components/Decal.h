#pragma once
#include "pch.h"
#include "../Object/Component.h"
#include "../Base/Datas/EnumData.hpp"

/*
    [ Decal ]

    Decal 데이터를 가지는 Component
    게임 특성상 Ground에만 맵핑되도록 RenderPass가 설정되어있으며
    해당 컴포넌트에서 이펙팅을 할지, Texture Mapping을 할지에 대한 데이터를 저장합니다.
*/
class Transform;

class Decal : public Component
{
    RTTR_ENABLE(Component)

public:
    DecalType type = DecalType::TextureMap;    // TextureMap, RingEffect
    std::string decalTexturePath = "";

    ComPtr<ID3D11ShaderResourceView> decalSRV = nullptr;

public:
    // component process
    Decal() = default;
    ~Decal() = default;
    void OnInitialize() override;
    void OnDestory() override;

    // json
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);

    void ChangeData(std::string path);
};

// util
std::wstring ToWString(const std::string& s);