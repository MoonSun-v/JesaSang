#include "Transform.h"
#include "../Manager/ComponentFactory.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Transform>("Transform")
		.constructor<>()
			(rttr::policy::ctor::as_std_shared_ptr)
		.property("Position",   &Transform::GetPosition,    &Transform::SetPosition)
		.property("Rotation",   &Transform::GetEuler,       &Transform::SetEuler)
		.property("Scale",      &Transform::GetScale,       &Transform::SetScale);
}

void Transform::OnUpdate(float delta)
{
    // dirty 해소
    if (dirty)
    {
        worldMatrix = Matrix::CreateScale(scale) *
                      Matrix::CreateFromYawPitchRoll(euler.y, euler.x, euler.z) *
                      Matrix::CreateTranslation(position);
        dirty = false;
    }
}

Matrix Transform::GetWorldTransform() const
{
    return worldMatrix;
}

void Transform::Translate(const Vector3& delta)   // TODO 매트릭스 스스로 업데이트 할 수 있게 변경하기
{
    position += delta;
    dirty = true;
}

void Transform::Rotate(const Vector3& delta)
{
    euler += delta;
    dirty = true;
}

nlohmann::json Transform::Serialize()
{
	nlohmann::json datas;

    rttr::type t = rttr::type::get(*this);
    datas["type"] = t.get_name().to_string();       
    datas["properties"] = nlohmann::json::object(); // 객체 생성

    for(auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);
	    if(value.is_type<DirectX::SimpleMath::Vector3>() && propName == "Position")
        {
            auto v = value.get_value<Vector3>();
            datas["properties"][propName] = {v.x, v.y, v.z};
        }
        else if(value.is_type<DirectX::SimpleMath::Vector3>() && propName == "Rotation")
        {
            auto v = value.get_value<Vector3>();
            datas["properties"][propName] = {v.x, v.y, v.z};
        }
        else if(value.is_type<DirectX::SimpleMath::Vector3>() && propName == "Scale")
        {
            auto v = value.get_value<Vector3>();
            datas["properties"][propName] = {v.x, v.y, v.z};
        }
	}

    return datas;
}

void Transform::Deserialize(nlohmann::json data)
{
    // data : data["objects"]["properties"]["components"]["현재 컴포넌트"]

    auto propData = data["properties"];

    rttr::type t = rttr::type::get(*this);
    for(auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);
	    if(value.is_type<DirectX::SimpleMath::Vector3>() && propName == "Position")
        {
            Vector3 vec = { propData["Position"][0], propData["Position"][1], propData["Position"][2] };
            prop.set_value(*this, vec);
            dirty = true;
        }
        else if(value.is_type<DirectX::SimpleMath::Vector3>() && propName == "Rotation")
        {
            Vector3 vec = { propData["Rotation"][0], propData["Rotation"][1], propData["Rotation"][2] };
            prop.set_value(*this, vec);
            dirty = true;
        }
        else if(value.is_type<DirectX::SimpleMath::Vector3>() && propName == "Scale")
        {
            Vector3 vec = { propData["Scale"][0], propData["Scale"][1], propData["Scale"][2] };
            prop.set_value(*this, vec);
            dirty = true;
        }
	}
}