#include "SceneSystem.h"

void SceneSystem::BeforUpdate()
{
	if(scenes.empty()) return;

	currentScene->CheckDestroy();
}

void SceneSystem::UpdateScene(float deltaTime)
{
	if(scenes.empty()) return; // 씬 없어서 터지는거 방지

	currentScene->OnUpdate(deltaTime);
}

std::shared_ptr<Scene> SceneSystem::GetSceneByIndex(int index)
{
	return scenes.find(index)->second;
}

void SceneSystem::AddScene()
{
	scenes.insert({scenes.size(), std::make_shared<Scene>()});
}

std::shared_ptr<Scene> SceneSystem::GetCurrentScene()
{
    return currentScene;
}

std::shared_ptr<Scene> SceneSystem::SetCurrentSceneByIndex(int i)
{
	currentScene = scenes.find(i)->second;
	return currentScene;
}
