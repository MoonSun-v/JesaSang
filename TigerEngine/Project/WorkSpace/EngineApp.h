#pragma once
#include "pch.h"
#include <map>
#include "GameApp.h"
#include "EngineSystem/SceneSystem.h"
#include "Renderer/ImguiRenderer.h"
#include "App/Editor.h"
#include "RendererPlatform/DirectX11Renderer.h"
#include "RenderQueue/RenderQueue.h"

#include "RenderPass/DirectionalLightPass.h"
#include "RenderPass/GBufferRenderPass.h"
#include "RenderPass/SkyboxRenderPass.h"
#include "RenderPass/ShadowRenderPass.h"
#include "RenderPass/DebugDrawPass.h"

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

    // renderpasses
    std::unique_ptr<ShadowRenderPass>       shadowPass{};
    std::unique_ptr<GBufferRenderPass>      gbufferPass{};
    std::unique_ptr<DirectionalLightPass>   directionalLightPass{};
    std::unique_ptr<SkyboxRenderPass>       skyboxPass{};


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

	std::shared_ptr<DirectX11Renderer> dxRenderer{};
    std::unique_ptr<RenderQueue> renderQueue{};
};