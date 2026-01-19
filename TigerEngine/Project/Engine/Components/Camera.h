#pragma once
#include "pch.h"
#include "System/InputSystem.h"
#include "../Object/Component.h"

using namespace DirectX::SimpleMath;

class Camera : public Component
{
	RTTR_ENABLE(Component)
public:
	Camera() { SetName("Camera"); } 
	~Camera() {};

	Vector3 GetForward();
	Vector3 GetRight();
	Matrix GetView();
	
	void SetProjection(float povAngle, int width, int height, float targetNear, float targetFar);
	Matrix GetProjection() const;

	void AddPitch(float value);
	void AddYaw(float value);
	void SetInputVec(const Vector3& inputVec);

	void OnInitialize() override;
	void OnStart() override;
	void OnUpdate(float delta) override;
    void OnDestory() override;

	float GetSpeed() const { return moveSpeed; }
	void SetSpeed(float value) {  moveSpeed = value; }

	float GetRotateSpeed() const { return rotSpeed; }
	void SetRotateSpeed(float value) { rotSpeed = value; }

	float GetPovAngle() const { return povAngle; }
	void SetPovAngle(float angle) { povAngle = angle; SetProjection(povAngle, screenWidth, screenHeight, nearDist, farDist); }

	float GetNearDist() const { return nearDist; }
	void SetNearDist(float value) { nearDist = value; SetProjection(povAngle, screenWidth, screenHeight, nearDist, farDist); }
	
	float GetFarDist() const { return farDist; }
	void SetFarDist(float value) { farDist = value; SetProjection(povAngle, screenWidth, screenHeight, nearDist, farDist); }

	nlohmann::json Serialize() override;
	void Deserialize(nlohmann::json data) override;

private:
	Matrix projection = Matrix::Identity;

	Vector3 inputVec{};
	float moveSpeed = 100.0f;
	float rotSpeed = 0.004f;

	float povAngle = DirectX::XM_PIDIV2;
	float nearDist = 0.01f;
	float farDist = 2000.0f;

    int screenWidth = 0;
    int screenHeight = 0;

    bool isMainCamera = false;
};