#pragma once
#include "pch.h"
#include "../Components/Transform.h"
#include <string> 
#include <vector>
#include "../Scene/Scene.h"
#include "System/ObjectSystem.h"
#include "../EngineSystem/RenderSystem.h"
#include "../EngineSystem/ScriptSystem.h"

class RenderComponent; // NOTE : Component 있는 거랑 순환 참조 조심하기

/// <summary>
/// GameObject는 컴포넌트를 담고 있는 순수한 컨테이너
/// </summary>
class GameObject : public Object
{
public:
	GameObject() { Initialize(); };
	GameObject(Scene* scene, std::string name) : GameObject()
	{
		this->name = name;
		SetScene(scene);
	}

	template<typename T>
	T* AddComponent();

	template<typename T>
	T* GetComponent();

	void RemoveComponent(Component* comp);

	Transform* GetTransform() const;
	std::vector<Component*> GetComponents();

	bool IsDestory();
	void Destory();

	std::string GetName() const;
	void SetName(std::string str);

	Scene* GetScene();
	/// @brief 게임 오브젝트가 포함될 씬 설정 함수
	void SetScene(Scene* scene);

	/// @brief 데이터 저장을 위한 직렬화 데이터 반환 함수
	/// @return 직렬화된 json 객체
	nlohmann::json Serialize() const;
	void Deserialize(const nlohmann::json objData);

	/// @brief 에디터 오브젝트 피킹을 위한 AABB 업데이트
	void UpdateAABB();
	const BoundingBox GetAABB() const { return aabbBox; }
	void SetAABB(BoundingBox aabb);
	void SetAABB(Vector3 min, Vector3 max, Vector3 centor);

	std::string name = "NoNamed";	// 리플렉션을 위해 public으로 공개

	/// <summary>
	/// 게임 오브젝트의 모든 컴포넌트를 지우는 함수
	/// </summary>
	void ClearAll();

protected:
	Scene* currentScene{}; // 현재 게임 오브젝트가 존재하는 씬 참조 변수
	Transform* transform{};
	std::vector<Component*> components;
	std::vector<Handle> handles;
	bool isDestory = false;

	BoundingBox aabbBox{};
	Vector3 aabbBoxExtent{};
	Vector3 aabbCenter{};

	void Initialize();
};

template <typename T>
inline T* GameObject::AddComponent()
{
	static_assert(std::is_base_of_v<Component, T>,
		"T must inherit from Component"); // T는 Component를 상속받았는가? 

	Handle handle = ObjectSystem::Instance().Create<T>();
	auto comp = ObjectSystem::Instance().Get<T>(handle);
	comp->SetOwner(this);
	components.push_back(comp);
	handles.push_back(handle);
	comp->OnInitialize(); // 컴포넌트 초기화 실행

 	if (auto renderComp = dynamic_cast<RenderComponent*>(comp))
	{
        // 렌더 컴포넌트
        RenderSystem::Instance().Register(renderComp);
	}
    else if(auto scriptComp = dynamic_cast<ScriptComponent*>(comp))
    {
        // 사용자 정의 컴포넌트
        ScriptSystem::Instance().RegisterScript(scriptComp);
    }
    else
    {
        // 엔진 기본 컴포넌트
        ScriptSystem::Instance().Register(comp);
    }

	return comp;
}

template <typename T>
inline T* GameObject::GetComponent()
{
	T* res = {};
	std::for_each(components.begin(), components.end(), [&res](auto comp)
	{
		if(typeid(*comp) == typeid(T)) res = dynamic_cast<T*>(comp);
	});

    return res;
}
