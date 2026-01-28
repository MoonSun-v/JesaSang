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
    void OnDestory() override;

	Matrix GetWorldTransform() const;
	void Translate(const Vector3& delta);
    void Rotate(const Vector3& delta);

	nlohmann::json Serialize() override;
	void Deserialize(nlohmann::json data) override;

    const Vector3& GetPosition() const { return position; }
    const Vector3& GetEuler() const { return euler; }
    const Quaternion& GetQuaternion() const { return quaternion; }
    const Vector3& GetScale() const { return scale; }
    const Matrix& GetWorldMatrix() const { return worldMatrix; }
    Matrix& GetLocalMatrix();

    void SetPosition(const Vector3& p)
    {
        position = p;
        dirty = true;
        SetChildrenDirty();
    }

    // 에디터/직렬화용
    void SetEuler(const Vector3& r)
    {
        euler = r;
        quaternion = Quaternion::CreateFromYawPitchRoll(euler.y, euler.x, euler.z);
        dirty = true;
        SetChildrenDirty();
    }

    // Physics/엔진 내부용
    void SetQuaternion(const Quaternion& q)
    {
        quaternion = q;
        quaternion.Normalize();
        euler = q.ToEuler();   
        dirty = true;
        SetChildrenDirty();
    }

    void SetScale(const Vector3& s)
    {
        scale = s;
        dirty = true;
        SetChildrenDirty();
    }

    // Y축 회전값 (Yaw) getter (rad)
    float GetYaw() const
    {
        return euler.y;
    }

    // Y축 회전만 설정 (rad)
    void SetRotationY(float yaw)
    {
        euler.y = yaw;
        quaternion = Quaternion::CreateFromYawPitchRoll(
            euler.y, euler.x, euler.z
        );
        dirty = true;
        SetChildrenDirty();
    }

    void AddChild(Transform* transPtr);
    void RemoveChild(Transform* transPtr);
    bool SetParent(Transform* transPtr);
    void RemoveChildren();
    void RemoveSelfAtParent();  // 부모에서 자신을 스스로 제거함
    void SetChildrenDirty();    // 모든 자식 dirty 플래그 활성화
    void SetDirty();            // dirty = true; 

    Transform* GetParent() const { return parent; }
    const std::vector<Transform*>& GetChildren() const { return children; }

private:
    Vector3 position{ Vector3::Zero };
    Vector3 euler{ Vector3::Zero };     // 오일러 각으로 표현한 라디안 값
    Quaternion quaternion{ Quaternion::Identity }; // 쿼터니언 
    Vector3 scale{ Vector3::One };

    Matrix worldMatrix{};
    Matrix localMatrix{};
    Transform* parent{};    // 이건 업데이트에서 확인하고 자동 제거
    std::vector<Transform*> children;

	bool dirty = true;
};

