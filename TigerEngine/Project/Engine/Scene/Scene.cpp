#include "Scene.h"	
#include "../Object/GameObject.h"
#include "System/ObjectSystem.h"
#include "../EngineSystem/ScriptSystem.h"

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
