#include "Scene.h"	
#include "../Entity/GameObject.h"
#include "../System/ObjectSystem.h"
#include "../System/ScriptSystem.h"

void Scene::OnRender(std::unique_ptr<RenderQueue>& renderQueue)
{
	for(auto it = gameObjects.begin(); it != gameObjects.end(); it++)
	{
		auto gameObject = it->second;
		// .. rendering
	}
}

void Scene::OnUpdate(float deltaTime)
{
    ScriptSystem::Instance().Update(deltaTime);
}

void Scene::CheckDestroy()
{
	for(auto it = gameObjects.begin(); it != gameObjects.end();)
	{
		auto gameObject = it->second;
		if(gameObject.objPtr->IsDestory())
		{				
			gameObject.objPtr->ClearAll();
			ObjectSystem::Instance().Destory(gameObject.handle);
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
		fn(obj.second.objPtr);
	}
}

GameObject* Scene::AddGameObjectByName(std::string name)
{
	Handle handle = ObjectSystem::Instance().Create<GameObject>();
	auto obj = ObjectSystem::Instance().Get<GameObject>(handle);
	obj->SetScene(this);
	obj->SetName(name);

	gameObjects.insert({ name, {obj, handle} });
    return obj;
}

GameObject* Scene::GetGameObjectByName(std::string name)
{
    return gameObjects.find(name)->second.objPtr;
}

void Scene::ClearScene()
{
	for(auto it = gameObjects.begin(); it != gameObjects.end();)
	{
		auto [ptr, handle] = it->second;

        ptr->ClearAll();
        ObjectSystem::Instance().Destory(handle);
        it = gameObjects.erase(it);
	}
	
	gameObjects.clear();
}

bool Scene::SaveToJson(const std::string &filename) const
{
	nlohmann::json root;

	root["objects"] = nlohmann::json::array();
	for(auto& obj : gameObjects)
	{
		if(!obj.second.objPtr) continue;
		
		nlohmann::json objData = obj.second.objPtr->Serialize();
		root["objects"].push_back(objData);
	}

	std::ofstream file(filename);
	if(!file.is_open()) return false;

	file << root.dump(2); // ??
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

	ClearScene();
	
	// json 데이터에 objects 객체이나 배열이 없음
	if(!root.contains("objects") || !root["objects"].is_array()) return false;

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
	}

    return true;
}

GameObject* Scene::GetGameobjectFromScene(std::string name)
{
	if(auto it = gameObjects.find(name); it != gameObjects.end())
	{
		return it->second.objPtr;
	}
	else
	{
		return nullptr;
	}
}

GameObject* Scene::RayCastGameObject(const Ray &ray, float *outDistance)
{
	GameObject* hitObject = nullptr;
	float minDistant = FLT_MAX;

	for(auto& [name, obj]: gameObjects)
	{
		if(!obj.objPtr) continue;

		float outDist = 0.0f;
		if(ray.Intersects(obj.objPtr->GetAABB(), outDist))
		{
			if(outDist < minDistant)
			{
				minDistant = outDist;
				hitObject = obj.objPtr;
			}
		}
	}

	if(outDistance != nullptr && hitObject)
	{
		*outDistance = minDistant;
	}

    return hitObject;
}
