#pragma once
#include "pch.h"
#include "../Object/Component.h"

/// <summary>
/// GameObject가 기본으로 가지고 있는 위치 정보
/// </summary>
/// <remarks>
/// 내부 연산은 Quaternion으로 하며 사용자 편의를 위해 Euler Getter/Setter도 제공합니다.
/// 모든 Euler의 단위는 Radian으로 통일됩니다.
/// Editor 모드에서만 Degree로 변환되며, 클라이언트가 사용할 때는 Degree -> Radian 변환 작업을 적용시키고 Set합니다.
/// Json 저장도 Radian 단위 입니다.
/// </remarks>
class Transform : public Component
{
	RTTR_ENABLE(Component)
public:
	Transform() { SetName("Transform"); }
	~Transform() = default;

	void OnUpdate(float delta) override;
    void OnDestory() override;

	void Translate(const Vector3& delta);
    void Rotate(const Vector3& delta);

	nlohmann::json Serialize() override;
	void Deserialize(nlohmann::json data) override;

    // Position
    const Vector3& GetLocalPosition();
    Vector3 GetWorldPosition();
    void SetPosition(const Vector3& p);

    // 회전 ( Euler, Quaternion )
    const Vector3& GetEuler() const { return euler; }
    void SetEuler(const Vector3& rad);            // Euler : 주로 클라이언트가 편하게 계산하기 위해 사용
    const Quaternion& GetQuaternion() const { return quaternion; }
    void SetQuaternion(const Quaternion& quat);    // Quaternion : 엔진 내부 계산할 때 사용 
    float GetYaw() const; // Y축 회전값 (Yaw) getter (rad)

    // Scale
    const Vector3& GetScale() const { return scale; }
    void SetScale(const Vector3& scal);

    // Matrix
    Matrix& GetWorldMatrix();
    Matrix& GetLocalMatrix();
    
    void SetRotationY(float yaw); // Y축 회전만 설정 (rad)

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
    /// <summary>
    /// dirty flag 해소 함수
    /// </summary>
    void UpdateMatricesIfDirty();

    Vector3 position{ Vector3::Zero };  // local Position
    Vector3 euler{ Vector3::Zero };     // 오일러 각으로 표현한 라디안 값, local Position
    Quaternion quaternion{ Quaternion::Identity }; // 쿼터니언 
    Vector3 scale{ Vector3::One };

    Matrix worldMatrix{};
    Matrix localMatrix{};
    Transform* parent{};    // 이건 업데이트에서 확인하고 자동 제거
    std::vector<Transform*> children;

	bool dirty = true;
};

