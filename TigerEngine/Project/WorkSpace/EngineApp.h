#pragma once
#include "pch.h"
#include "GameApp.h"
#include "System/SceneSystem.h"
#include "Renderer/ImguiRenderer.h"
#include "Renderer/RenderQueue.h"
#include "App/Editor.h"
#include <map>
#include "Renderer/DirectX11Renderer.h"

/// <summary>
/// 렌더 파이프라인이 흐름을 관리하는 앱
/// </summary>
class EngineApp : public GameApp
{
public:
	EngineApp(HINSTANCE hInstance);
	~EngineApp();

	bool OnInitialize() override;
	void OnUpdate() override;
	void OnRender() override;

#if _DEBUG
	std::unique_ptr<ImguiRenderer> imguiRenderer{};
	std::unique_ptr<Editor> editor{};
#endif
	
	vector<std::shared_ptr<IRenderPass>> renderPasses;

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
	void OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker,
		const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker) override;

private:

	/// <summary>
	/// ComponentFactory에 사용하는 컴포넌트 등록하는 함수
	/// </summary>
	void RegisterAllComponents();

	/// <summary>
	/// 매 프레임마다 렌더링 시작할 때 호출
	/// </summary>
	void BeginRender();

	/// <summary>
	/// 매 프레임 마다 렌더링 끝날 때 호출
	/// </summary>
	void EndRender();

    void LoadSavedFirstScene();

    std::optional<std::filesystem::path> FindDirectoryInAssets(std::string dirName);
    std::filesystem::path GetExeDir();

	std::shared_ptr<DirectX11Renderer> dxRenderer{};
};