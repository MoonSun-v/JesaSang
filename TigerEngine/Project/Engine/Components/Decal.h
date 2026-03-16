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
    
    // opacity
    float opacity = 1.0f;

    // ground decal?
    bool isGroundDecal = false;

    // ground
    float upThreshold = 0.9f;

    // decal texture
    Vector2 tiling = { 1.0f, 1.0f };
    Vector2 offset = { 0.0f, 0.0f };
    std::string decalTexturePath = "";
    ComPtr<ID3D11ShaderResourceView> decalSRV = nullptr;

    // ring effect
    float ringStartTime = 0.0f;     // 스폰 시점
    float ringDuration  = 10.0f;    // 링 수명
    float ringMaxRadius = 0.95f;    // 0~1(uv공간 반경)
    float ringSpeed     = 0.45f;    // Speed
    float ringThickness = 0.005f;   // 링 두께 (uv공간)
    float ringFeather   = 0.01f;    // 링 가장자리 소프트
    Vector3 ringColor   = { 0.0f, 0.0f, 1.0f }; 

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

    void Enable_Inner() override;
    void Disable_Inner() override;

    // ring effect func
    void StartRingEffect(float maxRadiusRate, float startTime, float duration = 10.0f , float speed = 0.3f);
};

// util
std::wstring ToWString(const std::string& s);