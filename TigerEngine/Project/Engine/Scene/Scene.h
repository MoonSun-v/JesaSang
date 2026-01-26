#pragma once
#include "pch.h"
#include "../Components/RenderComponent.h"
#include <map>

class GameObject;

struct GameObjectEntity
{
	GameObject* objPtr;
	Handle handle;
};

/// <summary>
/// 게임 오브젝트들의 생명을 관리하는 객체
/// </summary>
class Scene
{
public:
	void OnUpdate(float deltaTime);	

    void OnFixedUpdate(float dt);

	/// <summary>
	/// IsDestroy 플래그 확인 함수 ( 활성화 되었으면 해당 오브젝트는 다음 프레임에 파괴됨 )
	/// </summary>
	void CheckDestroy();

	/// <summary>
	/// 모든 게임오브젝트에게 std::function을 호출하게하는 함수
	/// </summary>
	/// <param name="fn">매개변수가 GameObect*인 funtor</param>
	void ForEachGameObject(std::function<void(GameObject*)> fn);

	/// <summary>
	/// 이름으로 게임 오브젝트 등록
	/// </summary>
	/// <param name="name">게임 오브젝트의 이름</param>
	/// <returns>등록한 게임 오브젝트의 포인터</returns>
	GameObject* AddGameObjectByName(std::string name); // add empty gameObject to Scene

	/// <summary>
	/// 게임 오브젝트를 이름으로 찾기 ( 먼저 등록된 오브젝트 반환 )
	/// </summary>
	/// <param name="name">찾을 이름</param>
	/// <returns>같은 이름 중 먼저 등록된 오브젝트 없으면 nullptr</returns>
	GameObject* GetGameObjectByName(std::string name);

    /// <summary>
    /// 게임 오브젝트 찾기 ( 포인터로 찾음 )
    /// </summary>
    /// <param name="obj">찾을 게임 오브젝트 포인터</param>
    /// <returns>같은 주소의 게임 오브젝트 없으면 nullptr</returns>
    GameObject* GetGameObject(GameObject* obj);

	/// @brief 모든 씬 오브젝트들을 제거하는 함수
	void ClearScene();

	/// @brief json으로 scene정보를 저장하는 함수
	/// @param filename 저장할 파일 이름
	bool SaveToJson(const std::string& filename) const;

	/// @brief json 파일을 읽어서 scene에 로드하는 함수
	/// @param 불러오는 파일 이름
	bool LoadToJson(const std::string& filename);
	
	int GetObjectCount() { return gameObjects.size(); }

	/// <summary>
	/// Ray로 충돌된 게임 오브젝트 찾기
	/// </summary>
	/// <param name="ray">ray 객체</param>
	/// <param name="outDistance">[out param] ray 충돌 시 거리</param>
	/// <returns>ray와 충돌된 게임 오브젝트</returns>
	GameObject* RayCastGameObject(const Ray& ray, float* outDistance);

    /// <summary>
    /// 가장 최근에 로드된 json 파일을 다시 로드하는 함수
    /// </summary>
    void ReloadScene();

    /// <summary>
    /// 게임 오브젝트 포인터로 인덱스 찾기
    /// </summary>
    /// <param name="ptr">게임 오브젝트 포인터</param>
    /// <returns>배열 인덱스, 없으면 -1</returns>
    int GetGameObjectIndex(GameObject* ptr);

protected:
    /// <summary>
    /// 씬에서 사용하는 오브젝트 모음 
    /// </summary>
    std::vector<GameObjectEntity> gameObjects;

    /// <summary>
    /// 오브젝트를 찾을 때 사용하는 매핑 자료구조
    /// </summary>
    std::unordered_map<std::string, std::vector<std::pair<Handle, int>>> mappedGameObjects;

    std::string targetLoadedPath = "";
};