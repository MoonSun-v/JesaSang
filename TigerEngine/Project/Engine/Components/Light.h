#pragma once
#include "pch.h"
#include "../Object/Component.h"
#include "../Base/Datas/EnumData.hpp"

/*
    [ Light ]

    Directional, Point, Spot 라이트 데이터를 가지는 Component
*/

class Light : public Component
{
    RTTR_ENABLE(Component)

public:
    LightType type;
    bool isSunLight = false;    // only one

    Vector3   color;
    float     intensity;

    Vector3   direction;        // directional, spot
    float     range;            // point, spot
    float     innerAngle;       // spot
    float     outerAngle;       // spot

    // get/set
    Color GetColor() { return Color(color.x, color.y, color.z); }
    void SetColor(Color color) { this->color = { color.R(), color.G(), color.B() }; }

public:
    // component process
    Light();
    ~Light() override = default;
    void OnInitialize() override;
    void OnDestory() override;

    // json
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);
};