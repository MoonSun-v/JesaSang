#pragma once
#include "pch.h"
#include "../Base/Datas/EnumData.hpp"

class JsonHelper
{
public:
    template<typename T>
    static nlohmann::json MakeSaveData(const T* typePtr);

    template<typename T>
    static void SetDataFromJson(T* typePtr, nlohmann::json data);
};

template<typename T>
nlohmann::json JsonHelper::MakeSaveData(const T* typePtr)
{
    nlohmann::json datas;

    rttr::type t = rttr::type::get(*typePtr);
    datas["type"] = t.get_name().to_string();
    datas["properties"] = nlohmann::json::object(); // 객체 생성

    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*typePtr);

        if (!value.is_valid())
            continue;

        if (value.is_type<float>())
        {
            auto v = value.get_value<float>();
            datas["properties"][propName] = v;
        }
        else if (value.is_type<int>())
        {
            auto v = value.get_value<int>();
            datas["properties"][propName] = v;
        }
        else if (value.is_type<bool>())
        {
            auto v = value.get_value<bool>();
            datas["properties"][propName] = v;
        }
        else if (value.is_type<Color>())
        {
            auto v = value.get_value<Color>();
            datas["properties"][propName] = { v.x, v.y, v.z, v.w };
        }
        else if (value.is_type<std::string>())
        {
            auto v = value.get_value<std::string>();
            datas["properties"][propName] = v;
        }
        else if (value.is_type<Vector2>())
        {
            Vector2 v = value.get_value<SimpleMath::Vector2>();
            datas["properties"][propName] = { v.x, v.y };
        }
        else if (value.is_type<Vector3>())
        {
            Vector3 v = value.get_value<SimpleMath::Vector3>();
            datas["properties"][propName] = { v.x, v.y, v.z };
        }
        else if (value.is_type<DirectX::SimpleMath::Quaternion>())
        {
            auto v = value.get_value<DirectX::SimpleMath::Quaternion>();
            datas["properties"][propName] = { v.x, v.y, v.z, v.w };
        }
        else if (value.is_type<DirectX::SimpleMath::Matrix>())
        {
            auto v = value.get_value<DirectX::SimpleMath::Matrix>();    // NOTE : 이거 사용할 순간 오면 테스트 한 번 해야할 듯
            std::vector<float> mat;
            const float* p = (const float*)&v;
            mat.assign(p, p + 16);
            datas["properties"][propName] = mat;
        }
        // enum class string save
        else if (prop.get_type().is_enumeration())
        {
            std::string enumName = value.to_string();

            if (!enumName.empty())
                datas["properties"][propName] = enumName;
            else
                datas["properties"][propName] = nullptr;
        }
    }

    return datas;
}

template<typename T>
inline void JsonHelper::SetDataFromJson(T* typePtr, nlohmann::json data)
{
    auto propData = data["properties"];

    rttr::type t = rttr::type::get(*typePtr);
    for (auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*typePtr);
        if (!propData.contains(propName)) continue;

        if (value.is_type<float>())
        {
            float data = propData[propName];
            prop.set_value(*typePtr, data);
        }
        else if (value.is_type<int>())
        {
            int data = propData[propName];
            prop.set_value(*typePtr, data);
        }
        else if (value.is_type<bool>())
        {
            bool data = propData[propName];
            prop.set_value(*typePtr, data);
        }
        else if (value.is_type<Color>())
        {
            Color color = { propData[propName][0], propData[propName][1], propData[propName][2], propData[propName][3] };
            prop.set_value(*typePtr, color);
        }
        else if (value.is_type<std::string>())
        {
            std::string data = propData[propName];
            prop.set_value(*typePtr, data);
        }
        else if (value.is_type<SimpleMath::Vector2>())
        {
            Vector2 value = { propData[propName][0], propData[propName][1] };
            prop.set_value(*typePtr, value);
        }
        else if (value.is_type<SimpleMath::Vector3>())
        {
            Vector3 value = { propData[propName][0], propData[propName][1], propData[propName][2] };
            prop.set_value(*typePtr, value);
        }
        else if (value.is_type<DirectX::SimpleMath::Quaternion>())
        {
            if (propData[propName].size() == 4)
            {
                DirectX::SimpleMath::Quaternion q = { propData[propName][0], propData[propName][1], propData[propName][2], propData[propName][3] };
                prop.set_value(*typePtr, q);
            }
        }
        else if (value.is_type<DirectX::SimpleMath::Matrix>())
        {
            if (propData[propName].size() == 16)    // NOTE : 이거 사용할 순간 오면 테스트 해봐야할 듯
            {
                DirectX::SimpleMath::Matrix m;
                float* p = (float*)&m;
                for (int i = 0; i < 16; ++i) p[i] = propData[propName][i];
                prop.set_value(*typePtr, m);
            }
        }
        // enum string load
        else if (prop.get_type().is_enumeration())
        {
            if (!propData[propName].is_string())
                continue;

            std::string enumStr = propData[propName];

            auto enumType = prop.get_type().get_enumeration();
            auto enumVar = enumType.name_to_value(enumStr);

            if (enumVar.is_valid())
                prop.set_value(*typePtr, enumVar);
        }
    }
}
