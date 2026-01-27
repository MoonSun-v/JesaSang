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
    if (parent && parent->GetOwner()->IsDestory())
    {
        parent = nullptr; // nullptr로 만들기
    }

    // dirty 해소
    if (IsDirty())
    {
        if (!parent)
        {
            worldMatrix = Matrix::CreateScale(scale) *
                          // Matrix::CreateFromYawPitchRoll(euler.y, euler.x, euler.z) *
                          Matrix::CreateFromQuaternion(quaternion) *
                          Matrix::CreateTranslation(position);
        }
        else
        {
            worldMatrix = Matrix::CreateScale(scale) *
                // Matrix::CreateFromYawPitchRoll(euler.y, euler.x, euler.z) *
                Matrix::CreateFromQuaternion(quaternion) *
                Matrix::CreateTranslation(position) * 
                parent->GetWorldTransform();
        }

        dirty = false;
    }
}

void Transform::OnDestory()
{
    RemoveSelfAtParent();
}

Matrix Transform::GetWorldTransform() const
{
    //if (dirty)
    //{
    //    cachedMatrix = Matrix::CreateScale(scale) *
    //        // Matrix::CreateFromYawPitchRoll(euler.y, euler.x, euler.z) *
    //        Matrix::CreateFromQuaternion(quaternion) *
    //        Matrix::CreateTranslation(position);
    //}

    return worldMatrix;
}

void Transform::Translate(const Vector3& delta)
{
    position += delta;
    dirty = true;
}

void Transform::Rotate(const Vector3& delta)
{
    //euler += delta;
    //dirty = true;

    SetEuler(euler + delta);
    dirty = true;
}

// 직렬화/역직렬화는 Euler 기준 
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

void Transform::AddChild(Transform* transPtr)
{
    if (transPtr == nullptr) return;
    children.push_back(transPtr);
}

void Transform::RemoveChild(Transform* transPtr)
{
    for (auto it = children.begin(); it != children.end();)
    {
        if (*it == transPtr)
        {
            children.erase(it);
            break;
        }
        else
        {
            it++;
        }
    }
}

void Transform::SetParent(Transform* newParent)
{
    if (newParent == this) return; // 사이클 방지

    // 기존 부모 child 목록에서 제거
    if (parent)
        parent->RemoveChild(this);

    parent = newParent;

    // 새 부모 child 목록에 추가
    if (parent)
        parent->AddChild(this);

    dirty = true;
}

void Transform::RemoveChildren()
{
    for (auto it = children.begin(); it != children.end();)
    {
        it = children.erase(it);
    }
}

void Transform::RemoveSelfAtParent()
{
    if(parent)
        parent->RemoveChild(this);
}

bool Transform::IsDirty()
{
    if (parent != nullptr)
    {
        return dirty || parent->IsDirty();
    }
    else
    {
        return dirty;
    }
}
