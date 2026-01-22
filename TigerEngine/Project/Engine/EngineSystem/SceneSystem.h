#pragma once
#include "pch.h"
#include "../Scene/Scene.h"
#include <map>
#include <vector>
#include "System/Singleton.h"

class SceneSystem : public Singleton<SceneSystem>
{
public:
	SceneSystem(token) {};
	~SceneSystem() = default;

	void BeforUpdate();
	void UpdateScene(float deltaTime);
	void FixedUpdateScene(float deltaTime);

	std::shared_ptr<Scene> GetSceneByIndex(int index);
	void AddScene();
	std::shared_ptr<Scene> GetCurrentScene();
	std::shared_ptr<Scene> SetCurrentSceneByIndex(int i = 0);
private:
	std::map<int, std::shared_ptr<Scene>> scenes;
	std::shared_ptr<Scene> currentScene{};
};

