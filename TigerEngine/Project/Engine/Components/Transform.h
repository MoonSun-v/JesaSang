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
    const Vector3& GetScale() const { return scale; }
    const Matrix& GetMatrix() const { return worldMatrix; }

    void SetPosition(const Vector3& p)
    {
        position = p;
        dirty = true;
    }

    void SetEuler(const Vector3& r)
    {
        euler = r;
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
    Vector3 scale{ Vector3::One };

    Matrix worldMatrix{};

	bool dirty = true;
};

