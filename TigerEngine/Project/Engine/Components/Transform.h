#pragma once
#include "pch.h"
#include "../Object/Component.h"

class Transform : public Component
{
	RTTR_ENABLE(Component)
public:
	Transform() { SetName("Transform"); }
	~Transform() = default;

	void OnUpdate(float delta) override;

	Matrix GetWorldTransform() const;
	void Translate(const Vector3& delta);
    void Rotate(const Vector3& delta);

	nlohmann::json Serialize() override;
	void Deserialize(nlohmann::json data) override;

    const Vector3& GetPosition() const { return position; }
    const Vector3& GetEuler() const { return euler; }
    const Quaternion& GetQuaternion() const { return quaternion; }
    const Vector3& GetScale() const { return scale; }
    const Matrix& GetMatrix() const { return worldMatrix; }

    void SetPosition(const Vector3& p)
    {
        position = p;
        dirty = true;
    }

    // 에디터/직렬화용
    void SetEuler(const Vector3& r)
    {
        euler = r;
        quaternion = Quaternion::CreateFromYawPitchRoll(euler.y, euler.x, euler.z);
        dirty = true;
    }

    // Physics/엔진 내부용
    void SetQuaternion(const Quaternion& q)
    {
        quaternion = q;
        quaternion.Normalize();
        euler = q.ToEuler();   
        dirty = true;
    }

    void SetScale(const Vector3& s)
    {
        scale = s;
        dirty = true;
    }

	//std::shared_ptr<Transform> parent{};

private:
    Vector3 position{ Vector3::Zero };
    Vector3 euler{ Vector3::Zero };     // 오일러 각으로 표현한 라디안 값
    Quaternion quaternion{ Quaternion::Identity }; // 쿼터니언 
    Vector3 scale{ Vector3::One };

    Matrix worldMatrix{};

	bool dirty = true;
};

