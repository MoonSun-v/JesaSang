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
#include "RenderPass/Renderer/UIRenderPass.h"
#include "RenderPass/Renderer/EffectPass.h"


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
    void OnLateUpdate() override;

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
    std::unique_ptr<UIRenderPass>             uiPass{};
    std::unique_ptr<EffectPass>               effectPass{};

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

    // [ GPU Profiler ] ------------------------------------
    // 렌더 패스별 GPU 시간을 Timestamp Query로 측정하고, 라이팅 방식 3단계를 비교한다.
    enum class LightingMode { ForwardOnly, DeferredFullScreen, DeferredLightVolume, Count };
    enum GpuPass { GP_Shadow, GP_Geometry, GP_Decal, GP_Light, GP_Skybox, GP_Forward,
                   GP_Effect, GP_Bloom, GP_PostProcess, GP_UI, GP_Count };
    LightingMode lightingMode = LightingMode::DeferredLightVolume;

    void RunPass(IRenderPass& pass, GpuPass id, Camera* cam);

#if _DEBUG
    static constexpr int kQueryLatency = 4;     // 결과를 4프레임 뒤에 읽어 GPU 대기(stall) 방지
    static constexpr int kAverageFrames = 120;  // 120프레임 평균으로 표시

    struct GpuFrameQuery
    {
        ComPtr<ID3D11Query> disjoint;
        ComPtr<ID3D11Query> start;
        ComPtr<ID3D11Query> stamps[GP_Count];
        int ids[GP_Count] = {};
        int count = 0;
        LightingMode mode = LightingMode::DeferredLightVolume;
        bool issued = false;
    };
    GpuFrameQuery gpuQueries[kQueryLatency];
    int gpuFrameIndex = 0;

    // 누적 (현재 모드)
    LightingMode accumMode = LightingMode::DeferredLightVolume;
    double accumPassMs[GP_Count] = {};
    bool accumPassUsed[GP_Count] = {};
    double accumFrameMs = 0.0;
    int accumFrames = 0;

    // 모드별 마지막 평균 결과
    struct GpuModeResult
    {
        bool valid = false;
        float frameMs = 0.0f;
        float passMs[GP_Count] = {};
        bool passUsed[GP_Count] = {};
        int volumeLights = 0;
        int fullScreenLights = 0;
    };
    GpuModeResult gpuResults[(int)LightingMode::Count];

    // --gpu-bench : 세 모드를 자동으로 돌며 측정 -> gpu_bench.txt 저장 후 종료
    bool gpuProfilerEnabled = false;    // --gpu-profiler 또는 --gpu-bench 실행 시에만 true
    bool gpuBench = false;
    int benchPublishes = 0;     // 현재 모드에서 평균이 나온 횟수 (첫 평균은 워밍업으로 버림)
    int benchExitFrames = -1;   // 저장 후 종료까지 남은 프레임 (캡처할 시간)
    void GpuBenchStep();
    void GpuBenchWrite();

    void GpuProfileBegin();
    void GpuProfileEnd();
    void RenderGpuProfiler();
#endif

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