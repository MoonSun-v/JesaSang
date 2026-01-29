#include "Transform.h"
#include "../Manager/ComponentFactory.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Transform>("Transform")
		.constructor<>()
			(rttr::policy::ctor::as_std_shared_ptr)
		.property("Position",   &Transform::GetLocalPosition,    &Transform::SetPosition)
		.property("Rotation",   &Transform::GetEuler,       &Transform::SetEuler)
		.property("Scale",      &Transform::GetScale,       &Transform::SetScale);
}

void Transform::OnUpdate(float delta)
{
    if (parent && parent->GetOwner()->IsDestory())
    {
        RemoveSelfAtParent();
        parent = nullptr;
        dirty = true;
        SetChildrenDirty();
    }

    // dirty 해소
    UpdateMatricesIfDirty();
}

void Transform::OnDestory()
{
    RemoveSelfAtParent();
}

void Transform::Translate(const Vector3& delta)
{
    position += delta;
    dirty = true;
    SetChildrenDirty();
}

void Transform::Rotate(const Vector3& delta)
{
    //euler += delta;
    //dirty = true;

    SetEuler(euler + delta);
    dirty = true;
    SetChildrenDirty();
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
    SetChildrenDirty();
}

Matrix& Transform::GetWorldMatrix()
{
    UpdateMatricesIfDirty();
    return worldMatrix;
}

Matrix& Transform::GetLocalMatrix()
{
    UpdateMatricesIfDirty();
    return localMatrix;
}

Vector3 Transform::GetWorldPosition()
{
    UpdateMatricesIfDirty();
    return worldMatrix.Translation();
}

const Vector3& Transform::GetLocalPosition()
{
    return position;
}

void Transform::SetPosition(const Vector3& pos)
{
    position = pos;
    dirty = true;
    SetChildrenDirty();
}

void Transform::SetEuler(const Vector3& rad)
{
    euler = rad;
    quaternion = Quaternion::CreateFromYawPitchRoll(euler.y, euler.x, euler.z);
    dirty = true;
    SetChildrenDirty();
}

void Transform::SetQuaternion(const Quaternion& q)
{
    quaternion = q;
    quaternion.Normalize();

    euler = q.ToEuler(); // ToEuler : Radian값 반환

    dirty = true;
    SetChildrenDirty();
}

void Transform::SetScale(const Vector3& s)
{
    scale = s;
    dirty = true;
    SetChildrenDirty();
}

float Transform::GetYaw() const
{
    return euler.y;
}

void Transform::SetRotationY(float yawRad)
{
    euler.y = yawRad;
    quaternion = Quaternion::CreateFromYawPitchRoll(
        euler.y, euler.x, euler.z
    );
    dirty = true;
    SetChildrenDirty();
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

bool Transform::SetParent(Transform* newParent)
{
    if (newParent == this) return false; // 사이클 방지
    if (newParent == parent) return false; // 동일한 부모면 무시

    // 기존 부모 child 목록에서 제거
    if (parent)
        parent->RemoveChild(this);

    parent = newParent;

    // 새 부모 child 목록에 추가
    if (parent)
        parent->AddChild(this);

    dirty = true;
    SetChildrenDirty();

    return true;
}

void Transform::RemoveChildren()
{
    for (auto it = children.begin(); it != children.end();)
    {
        (*it)->SetParent(nullptr); // 부모 제거
        it = children.erase(it);
    }
}

void Transform::RemoveSelfAtParent()
{
    if (parent)
    {
        parent->RemoveChild(this);  // 부모한테서 자신 제거
        parent = nullptr;           // 부모 제거
        dirty = true;
        SetChildrenDirty();
    }
}
    
void Transform::SetChildrenDirty()
{
    for (auto& child : children)
    {
        child->SetDirty();
        child->SetChildrenDirty();
    }
}

void Transform::SetDirty()
{
    dirty = true;
}

void Transform::UpdateMatricesIfDirty()
{
    if (!dirty) return;

    localMatrix =
        Matrix::CreateScale(scale) *
        Matrix::CreateFromQuaternion(quaternion) *
        Matrix::CreateTranslation(position);

    if (!parent)
        worldMatrix = localMatrix;
    else
        worldMatrix = localMatrix * parent->GetWorldMatrix(); 

    dirty = false;
}