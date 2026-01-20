#pragma once
#include "pch.h"
#include "../Object/Component.h"


// Light Type enum class
enum class LightType
{
    Directional = 0,
    Point = 1,
    Spot = 2
};

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
    Vector3   position;         // point, spot   // TODO :: gameobject transform

    float     range;            // point, spot
    float     innerAngle;       // spot
    float     outerAngle;       // spot

public:
    // component process
    Light();
    ~Light() = default;
    void OnInitialize() override;
    void OnDestory() override;

    // json
    nlohmann::json Serialize();
    void Deserialize(nlohmann::json data);
};

// Utils ------------
static nlohmann::json Vec3ToJson(const Vector3& v);
static Vector3 JsonToVec3(const nlohmann::json& j, const Vector3& fallback = Vector3::Zero);