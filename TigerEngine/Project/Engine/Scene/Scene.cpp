#include "Scene.h"	
#include "../Object/GameObject.h"
#include "../EngineSystem/ScriptSystem.h"
#include "../Manager/WorldManager.h"
#include "System/ObjectSystem.h"
#include "../EngineSystem/LightSystem.h"
#include "../EngineSystem/CameraSystem.h"

void Scene::OnUpdate(float deltaTime)
{
    ScriptSystem::Instance().Update(deltaTime);
}

void Scene::OnFixedUpdate(float dt)
{
    ScriptSystem::Instance().FixedUpdate(dt);
}

void Scene::CheckDestroy()
{
	for(auto it = gameObjects.begin(); it != gameObjects.end();)
	{
		auto entity = *it;
		if(entity.objPtr->IsDestory())
		{	
            // 맵 요소 제거
            auto container = mappedGameObjects[entity.objPtr->GetName()];
            for (auto mapContainerIt = container.begin(); mapContainerIt != container.end(); mapContainerIt++)
            {
                auto& [handle, index] = *mapContainerIt;
                if (entity.handle.generation == handle.generation &&
                    entity.handle.index == handle.index) // 같은 핸들 찾기
                {
                    container.erase(mapContainerIt);
                    break;
                }
            }

            // 벡터 요소 제거
			entity.objPtr->ClearAll();
			ObjectSystem::Instance().Destory(entity.handle);

			it = gameObjects.erase(it);
		}
		else
		{
			it++;
		}
	}
}

void Scene::ForEachGameObject(std::function<void(GameObject*)> fn)
{
	for(auto& obj : gameObjects)
	{
		fn(obj.objPtr);
	}
}

GameObject* Scene::AddGameObjectByName(std::string name)
{
	Handle handle = ObjectSystem::Instance().Create<GameObject>();
	auto obj = ObjectSystem::Instance().Get<GameObject>(handle);
	obj->SetScene(this);
	obj->SetName(name);

    int index = static_cast<int>(gameObjects.size());
    gameObjects.push_back({ obj, handle });
    mappedGameObjects[name].push_back({ handle, index });
    return obj;
}

GameObject* Scene::GetGameObjectByName(std::string name)
{
    if (auto it = mappedGameObjects.find(name); it != mappedGameObjects.end())
    {
        auto container = it->second;
        return gameObjects[container.front().second].objPtr; // 무조건 첫번째 반환
    }
    else
    {
        return nullptr; // 없음
    }
}

GameObject* Scene::GetGameObject(GameObject* obj)
{
    std::string name = obj->GetName();
    if (auto it = mappedGameObjects.find(name); it != mappedGameObjects.end())
    {
        auto container = it->second;
        for (auto& [handle, index] : container)
        {
            if (gameObjects[index].objPtr->GetName() == name)
            {
                return gameObjects[index].objPtr;
            }
        }
    }
    else
    {
        return nullptr; // 없음
    }
}

void Scene::ClearScene()
{
	for(auto it = gameObjects.begin(); it != gameObjects.end();)
	{
		auto [ptr, handle] = *it;

        ptr->ClearAll();
        ObjectSystem::Instance().Destory(handle);
        it = gameObjects.erase(it);
	}
	
	gameObjects.clear();
    mappedGameObjects.clear();
    LightSystem::Instance().Clear();
    CameraSystem::Instance().Clear();
}

bool Scene::SaveToJson(const std::string &filename) const
{
	nlohmann::json root;

    // 씬에 있는 게임 오브젝트 내용 저장
	root["objects"] = nlohmann::json::array();
	for(auto& entity : gameObjects)
	{
		if(!entity.objPtr) continue;
		
		nlohmann::json entityData = entity.objPtr->Serialize();
		root["objects"].push_back(entityData);
	}

    // 해당 씬의 월드 세팅 내용 저장
    root["worldData"] = nlohmann::json::array();
    auto& wm = WorldManager::Instance();    
    nlohmann::json worldData = wm.Serialize();
    root["worldData"].push_back(worldData);

    // 파일 만들기
	std::ofstream file(filename);
	if(!file.is_open()) return false;

	file << root.dump(2); // 보기 좋게 2칸 들여쓰기
	file.close();

	return true;
}

bool Scene::LoadToJson(const std::string &filename)
{
	std::ifstream file(filename);
	if(!file.is_open()) return false;

	nlohmann::json root;

	try
	{
		file >> root;
	}
	catch(const nlohmann::json::exception& e)
	{	// 파일 열기 실패
		MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
		file.close();
		return false;
	}
	file.close();	
	
	// json 데이터에 objects 객체이나 배열이 없음
	if(!root.contains("objects") || !root["objects"].is_array()) return false;

    ClearScene(); // 데이터가 존재하면 현재 씬 제거
    targetLoadedPath = filename;

    // 데이터에 있는 게임 오브젝트 불러오기
    vector<int> parentIDs; // 넣은 순서대로 부모 ID 기억하기 ( 부모가 없으면 -1 )
	for(const auto& objData : root["objects"])
	{
		if(!objData.contains("type")) continue;

		std::string typeName = objData["type"]; // 클래스 타입 : GameObject
		
		// 이름 찾기
		std::string objectName = "";
		if(objData.contains("properties")) objectName = objData["properties"]["Name"];

		auto instance = AddGameObjectByName(objectName);
		if(!instance) continue; 

		instance->Deserialize(objData["properties"]);

        if (objData["properties"].contains("ID"))
        {
            instance->SetId(objData["properties"]["ID"]); // ID 추가
        }        

        if (objData["properties"].contains("ParentID"))
        {
            parentIDs.push_back(objData["properties"]["ParentID"]); // 미리 부모 ID 기억
        }
	}

    // 월드 데이터 불러오기
    if (!root.contains("worldData") || !root["worldData"].is_array()) return false;

    auto& wm = WorldManager::Instance();
    wm.Deserialize(root["worldData"][0]);

    // 게임 오브젝트 계층 구조 맞추기

    // ObjectsID와 1:1 대응이라고 가정
    // id가 없는 데이터 이거나 데이터 개수가 안맞으면 무시한다.
    if (!parentIDs.empty() && parentIDs.size() == gameObjects.size())
    {
        for (int i = 0; i < gameObjects.size(); i++)
        {
            auto currObject = gameObjects[i].objPtr;
            int currParentID = parentIDs[i];
            if (currParentID == -1) continue; // 부모없음

            for (int j = 0; j < gameObjects.size(); j++)
            {
                if (i == j) continue;

                if (currParentID == gameObjects[j].objPtr->GetId()) // 부모 찾음
                {
                    currObject->GetTransform()->SetParent(gameObjects[j].objPtr->GetTransform()); // 부모 설정
                    break;
                }
            }
        }
    }

    // 새로운 ID 부여 -> 이전 Save데이터와 ID 충돌 방지
    for (auto& obj : gameObjects)
    {
        obj.objPtr->SetId(ObjectSystem::Instance().GetNewID());
    }

    return true;
}

GameObject* Scene::RayCastGameObject(const Ray &ray, float *outDistance)
{
	GameObject* hitObject = nullptr;
	float minDistant = FLT_MAX;

	for(auto& [ptr, handle]: gameObjects)
	{
		if(!ptr) continue;

		float outDist = 0.0f;
		if(ray.Intersects(ptr->GetAABB(), outDist))
		{
			if(outDist < minDistant)
			{
				minDistant = outDist;
				hitObject = ptr;
			}
		}
	}

	if(outDistance != nullptr && hitObject)
	{
		*outDistance = minDistant;
	}

    return hitObject;
}

void Scene::ReloadScene()
{
    ClearScene();
    LoadToJson(targetLoadedPath);
}

int Scene::GetGameObjectIndex(GameObject* ptr)
{
    int res = -1;
    for (int i = 0; i < gameObjects.size(); i++)
    {
        auto [objPtr, handle] = gameObjects[i];

        if (ptr == objPtr)
        {
            res = i;
            break;
        }
    }

    return res;
}

GameObject* Scene::GetGameObjectByIndex(int index)
{
    if (index < 0 || index >= gameObjects.size()) return nullptr;

	return gameObjects[index].objPtr;
}
