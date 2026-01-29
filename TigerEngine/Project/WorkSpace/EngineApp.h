#pragma once
#include "pch.h"
#include <map>
#include "GameApp.h"
#include "EngineSystem/SceneSystem.h"
#include "Renderer/ImguiRenderer.h"
#include "App/Editor.h"
#include "RendererPlatform/DirectX11Renderer.h"
#include "RenderQueue/RenderQueue.h"

#include "RenderPass/DebugDrawPass.h"
#include "RenderPass/Renderer/ShadowPass.h"
#include "RenderPass/Renderer/GeometryPass.h"
#include "RenderPass/Renderer/DecalPass.h"
#include "RenderPass/Renderer/LightPass.h"
#include "RenderPass/Renderer/SkyboxPass.h"
#include "RenderPass/Renderer/ForwardTransparentPass.h"
#include "RenderPass/Renderer/BloomPass.h"
#include "RenderPass/Renderer/PostProcessPass.h"
#include "RenderPass/Renderer/FrustumPass.h"


/// <summary>
/// 렌더 파이프라인이 흐름을 관리하는 앱
/// </summary>
class EngineApp : public GameApp
{
public:
	EngineApp(HINSTANCE hInstance);
	~EngineApp();

	bool OnInitialize() override;
    void OnPreUpdate() override;
	void OnUpdate() override;
	void OnRender() override;
    void OnFixedUpdate() override;

#if _DEBUG
	std::unique_ptr<ImguiRenderer> imguiRenderer{};
	std::unique_ptr<Editor> editor{};
#endif

    // [ Render Pass ] ---------------------------------
    std::unique_ptr<ShadowPass>               shadowPass{};
    std::unique_ptr<GeometryPass>             geometryPass{};
    std::unique_ptr<LightPass>                lightPass{};
    std::unique_ptr<DecalPass>                decalPass{};
    std::unique_ptr<SkyboxPass>               skyboxPass{};
    std::unique_ptr<ForwardTransparentPass>   forwardTransparentPass{};
    std::unique_ptr<BloomPass>                bloomPass{};
    std::unique_ptr<PostProcessPass>          postProcessPass{};
    std::unique_ptr<FrustumPass>              frustumPass{};
    // TODO :: Particle Pass


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

    // resize
    void ResizeScreen(int width, int height);
    void ResizeResource();

	std::shared_ptr<DirectX11Renderer> dxRenderer{};
    std::unique_ptr<RenderQueue> renderQueue{};

    void Woo_Registeration();
    void Moon_Registeration();
    void Ron_Registeration();
    void Ho_Registeration();
};