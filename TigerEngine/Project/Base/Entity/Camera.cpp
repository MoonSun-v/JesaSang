#include "Camera.h"
#include "GameObject.h"
#include "../System/ComponentFactory.h"
#include "../System/CameraSystem.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Camera>("Camera")
		.constructor<>()
			(rttr::policy::ctor::as_std_shared_ptr)
		.property("moveSpeed", 	&Camera::GetSpeed, 			&Camera::SetSpeed)
		.property("rotSpeed", 	&Camera::GetRotateSpeed,	&Camera::SetRotateSpeed)
		.property("nearDist", 	&Camera::GetNearDist, 		&Camera::SetNearDist)
		.property("farDist", 	&Camera::GetFarDist, 		&Camera::SetFarDist)
		.property("angle", 	    &Camera::GetPovAngle, 		&Camera::SetPovAngle);
}

Vector3 Camera::GetForward()
{
	Matrix world = owner->GetTransform()->GetWorldTransform();
	return -world.Forward();
}

Vector3 Camera::GetRight()
{
	Matrix world = owner->GetTransform()->GetWorldTransform();
    return world.Right();
}

Matrix Camera::GetView()
{
	Matrix world = owner->GetTransform()->GetWorldTransform();
	Vector3 eye = world.Translation();
	Vector3 target = eye + GetForward();
	Vector3 up = world.Up();

	return DirectX::XMMatrixLookAtLH(eye, target, up);
}

void Camera::AddPitch(float value)
{
    float eps = 0.0001f;
	auto rot = owner->GetTransform()->GetEuler();
	rot.x += value;
    if (rot.x > XM_PI)
    {
        rot.x -= XM_2PI;
    }
    else if (rot.x < -XM_PI)
    {
        rot.x += XM_2PI;
    }
    owner->GetTransform()->SetEuler(rot);
}

void Camera::AddYaw(float value)
{
    float eps = 0.0001f;
	auto rot = owner->GetTransform()->GetEuler();
	rot.y += value;
    if (rot.y > XM_PI)
    {
        rot.y -= XM_2PI;
    }
    else if (rot.y < -XM_PI)
    {
        rot.y += XM_2PI;
    }
    owner->GetTransform()->SetEuler(rot);
}

void Camera::SetInputVec(const Vector3 &inputVec)
{
	this->inputVec += inputVec;
	this->inputVec.Normalize();
}

void Camera::OnInitialize()
{
	InputSystem::Instance().Register(this);
    CameraSystem::Instance().Register(this);
    Vector2 screen = CameraSystem::Instance().GetScreenSize();
    screenWidth = screen.x;
    screenHeight = screen.y;

    SetProjection(povAngle, screenWidth, screenHeight, nearDist, farDist);
}

void Camera::OnStart()
{
}

void Camera::OnUpdate(float delta)
{
	auto& transform = *owner->GetTransform();
	if (inputVec.LengthSquared() > 0.0f)
	{
        transform.Translate(inputVec * moveSpeed * delta);
		inputVec = Vector3::Zero;
	}
}

void Camera::OnDestory()
{
    CameraSystem::Instance().RemoveCamera(this);
}

void Camera::SetProjection(float povAngle, int width, int height, float targetNear, float targetFar)
{
	projection = DirectX::XMMatrixPerspectiveFovLH(povAngle, width / (float)height, targetNear, targetFar);

	this->povAngle = povAngle;
	this->nearDist = targetNear;
	this->farDist = targetFar;
}

Matrix Camera::GetProjection() const
{
	return projection;
}

nlohmann::json Camera::Serialize()
{
	nlohmann::json datas;

    rttr::type t = rttr::type::get(*this);
    datas["type"] = t.get_name().to_string();       
    datas["properties"] = nlohmann::json::object(); // 객체 생성

    for(auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);
	    if(value.is_type<float>() && propName == "moveSpeed")
        {
            auto v = value.get_value<float>();
            datas["properties"][propName] = v;
        }
        else if(value.is_type<float>() && propName == "rotSpeed")
        {
            auto v = value.get_value<float>();
            datas["properties"][propName] = v;
        }
        else if(value.is_type<float>() && propName == "nearDist")
        {
            auto v = value.get_value<float>();
            datas["properties"][propName] = v;
        }
		else if(value.is_type<float>() && propName == "farDist")
        {
            auto v = value.get_value<float>();
            datas["properties"][propName] = v;
        }
	}

    return datas;
}

void Camera::Deserialize(nlohmann::json data)
{
	// data : data["objects"]["properties"]["components"]["현재 컴포넌트"]

    auto propData = data["properties"];

    rttr::type t = rttr::type::get(*this);
    for(auto& prop : t.get_properties())
    {
        std::string propName = prop.get_name().to_string();
        rttr::variant value = prop.get_value(*this);
	    if(value.is_type<float>() && propName == "moveSpeed")
        {
            auto v = propData["moveSpeed"];
            prop.set_value(*this, v);
        }
        else if(value.is_type<float>() && propName == "rotSpeed")
        {
            auto v = propData["rotSpeed"];
            prop.set_value(*this, v);
        }
        else if(value.is_type<float>() && propName == "nearDist")
        {
            auto v = propData["nearDist"];
            prop.set_value(*this, v);
        }
		else if(value.is_type<float>() && propName == "farDist")
        {
            auto v = propData["farDist"];
            prop.set_value(*this, v);
        }
	}
}

void Camera::OnInputProcess(const Keyboard::State &KeyState, const Keyboard::KeyboardStateTracker &KeyTracker, const Mouse::State &MouseState, const Mouse::ButtonStateTracker &MouseTracker)
{
	Vector3 forward = GetForward();
	Vector3 right = GetRight();

	if (KeyTracker.IsKeyPressed(Keyboard::Keyboard::R))
	{
		//Reset();
	}

	if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::W))
	{
		SetInputVec(forward);
	}
	else if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::S))
	{
		SetInputVec(-forward);
	}

	if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::A))
	{
		SetInputVec(-right);
	}
	else if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::D))
	{
		SetInputVec(right);
	}

	if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::E))
	{
		Matrix world = owner->GetTransform()->GetWorldTransform();		
		SetInputVec(-world.Up());
	}
	else if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::Q))
	{
		Matrix world = owner->GetTransform()->GetWorldTransform();		
		SetInputVec(world.Up());
	}

	// 속도 추가
	if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::D1))
	{
		moveSpeed = 20;
	}
	if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::D2))
	{
		moveSpeed = 100;
	}
	if (KeyState.IsKeyDown(DirectX::Keyboard::Keys::D3))
	{
		moveSpeed = 200;
	}

	Singleton<InputSystem>::Instance().m_Mouse->SetMode(MouseState.rightButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);
	if (MouseState.positionMode == Mouse::MODE_RELATIVE)
	{
		Vector3 delta = Vector3(float(MouseState.x), float(MouseState.y), 0.f) * rotSpeed;
		AddPitch(delta.y);
		AddYaw(delta.x);
	}
}
