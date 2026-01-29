#pragma once
#include "pch.h"
#include "System/ObjectSystem.h"

class GameObject; // forward declear

class Component : public Object
{
	RTTR_ENABLE()
protected:
	GameObject* owner{}; 	// 해당 컴포넌트를 소유하고 있는 게임 오브젝트
	std::string name{};		// 해당 컴포넌트 이름

    bool isStart = false;   // OnStart를 호출 했는가?


public:
	Component() = default;
	virtual ~Component() = default;
	
	/// <summary>
	/// Component가 처음 실행될 때 실행됩니다.
	/// </summary>
	virtual void OnInitialize() {};

	/// <summary>
	/// OnUpdate()를 실행하기 전 ***한 번*** 실행됩니다.
	/// </summary>
	virtual void OnStart() {};

	/// <summary>
	/// GameEngine에 매 프레임마다 OnUpdate()내 에서 호출됩니다.
	/// </summary>
	virtual void OnUpdate(float delta) {};

    /// <summary>
    /// 
    /// </summary>
    virtual void OnFixedUpdate(float delta) {};

    /// <summary>
    /// 우정추가. 렌더링이 끝난 뒤 호출(카메라 이동 등에 사용)
    /// </summary>
    virtual void OnLateUpdate(float delta) {};

    /// <summary>
    /// 객체가 파괴 될 때 호출됩니다.
    /// </summary>
    virtual void OnDestory() {};

	// 컴포넌트 내용 직렬화해서 json으로 반환하는 함수
	virtual nlohmann::json Serialize() { return nlohmann::json::object(); }
	virtual void Deserialize(nlohmann::json data) {};

	// 컴포넌트에 부착된 게임 오브젝트 찾기 ( getter )
	GameObject* GetOwner() { return owner; }

	// 컴포넌트에 부착된 게임 오브젝트 설정 ( setter )
	void SetOwner(GameObject* obj) { owner = obj; }

	// 컴포넌트 찾기를 위한 이름 getter
	std::string GetName() const { return name; }

	// 컴포넌트 찾기를 위한 이름 setter
	void SetName(std::string str) { name = str; }

    /// <summary>
    /// 플레이 모드에서만 실행가능 여부
    /// </summary>
    /// <returns>true면 플레이 모드에서만 update 호출 아니면 false</returns>
    virtual bool IsPlayModeOnly() { return false; }

    bool IsStart() { return isStart; }
    void SetStartTrue() { isStart = true; }
};