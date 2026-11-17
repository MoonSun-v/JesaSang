#if _DEBUG
#include "imgui_impl_win32.h" // ImGui_ImplWin32_WndProcHandler 사용하기 위함
#endif

#include "GameApp.h"
#include "EngineApp.h"


#include "Manager/FBXResourceManager.h"
#include "Manager/AudioManager.h"
#include "Manager/ShaderManager.h"
#include "Manager/WorldManager.h"
#include "Manager/UIManager.h"
#include "Manager/TextureResourceManager.h"
#include "Manager/UIManager.h"

#include "Entity/Object.h"
#include "Object/GameObject.h"

#include "System/ObjectSystem.h"
#include "EngineSystem/CameraSystem.h"
#include "EngineSystem/PlayModeSystem.h"
#include "EngineSystem/LightSystem.h"
#include "EngineSystem/PhysicsSystem.h"
#include "EngineSystem/AnimationSystem.h"
#include "EngineSystem/DecalSystem.h"
#include "EngineSystem/GridSystem.h"
#include "EngineSystem/AgentSystem.h"
#include "EngineSystem/EffectSystem.h"

#include "Components/FBXData.h"

namespace fs = std::filesystem;

EngineApp::EngineApp(HINSTANCE hInstance)
    : GameApp(hInstance)
{
}

EngineApp::~EngineApp()
{
    PhysicsSystem::Instance().Shutdown();
    AudioManager::Instance().Shutdown();
}

bool EngineApp::OnInitialize()
{
    RegisterAllComponents();

    // == init renderer ==
    dxRenderer = std::static_pointer_cast<DirectX11Renderer>(renderer);
#if _DEBUG
    imguiRenderer = std::make_unique<ImguiRenderer>();
    imguiRenderer->Initialize(hwnd, dxRenderer->GetDevice(), dxRenderer->GetDeviceContext());
#endif

    // == init system ==
    FBXResourceManager::Instance().GetDevice(dxRenderer->GetDevice(), dxRenderer->GetDeviceContext());
    ShaderManager::Instance().Init(dxRenderer->GetDevice(), dxRenderer->GetDeviceContext(), clientWidth, clientHeight);
    AudioManager::Instance().Initialize();
    // AudioManager::Instance().GetSystem().Set3DSettings(1.0f, 0.01f, 1.0f);// 기본 3D 설정 도플러 스케일, 거리 , 감쇠 효과
    AudioManager::Instance().GetSystem().Set3DSettings(0.0f, 0.01f, 1.0f);
    if (!PhysicsSystem::Instance().Initialize()) { return false; }
    // GridSystem::Instance().Initialize();

    auto& sm = ShaderManager::Instance();
    sm.viewport_screen = dxRenderer->GetRenderViewPort();
    sm.backBufferRTV = dxRenderer->GetBackBufferRTV();
    sm.depthStencilView = dxRenderer->GetDepthStencilView();
    sm.depthStencilReadOnlyView = dxRenderer->GetDepthStencilReadOnlyView();
    sm.depthSRV = dxRenderer->GetDepthSRV();
    sm.device = dxRenderer->GetDevice();
    sm.deviceContext = dxRenderer->GetDeviceContext();

    UIManager::Instance().SetSize(clientWidth, clientHeight);
    UIManager::Instance().Init(dxRenderer->GetDevice(), dxRenderer->GetDeviceContext());
    TextureResourceManager::Instance().Init(dxRenderer->GetDevice(), dxRenderer->GetDeviceContext());

    renderQueue = std::make_unique<RenderQueue>();

#if _DEBUG
    editor = std::make_unique<Editor>();
    editor->GetScreenSize(clientWidth, clientHeight);
    editor->Initialize(dxRenderer->GetDevice(), dxRenderer->GetDeviceContext());
    editor->GetDSV(dxRenderer->GetDepthStencilView());
    editor->GetRTV(dxRenderer->GetBackBufferRTV());
    editor->CreatePickingStagingTex();
#endif

    SceneSystem::Instance().AddScene();			    	// create first scene
    SceneSystem::Instance().SetCurrentSceneByIndex(); 	// render first scene

    // create free camera
    CameraSystem::Instance().SetScreenSize(clientWidth, clientHeight);

    // == find scene ==
    LoadSavedFirstScene(); // SceneClear 호출해서 객채 생성은 이 코드 이후에 해야함 

#if _DEBUG
    auto freeCamHandle = CameraSystem::Instance().CreateFreeCamera(clientWidth, clientHeight, SceneSystem::Instance().GetCurrentScene().get());

    // --cam=x,y,z,pitchDeg,yawDeg : 프리 카메라 시작 위치 (GPU 벤치마크를 같은 시점에서 반복하기 위함)
    if (const wchar_t* arg = wcsstr(GetCommandLineW(), L"--cam="))
    {
        float v[5] = {};
        if (swscanf_s(arg + 6, L"%f,%f,%f,%f,%f", &v[0], &v[1], &v[2], &v[3], &v[4]) == 5)
        {
            auto* t = CameraSystem::Instance().GetFreeCamera()->GetOwner()->GetTransform();
            t->SetPosition({ v[0], v[1], v[2] });
            t->SetEuler({ XMConvertToRadians(v[3]), XMConvertToRadians(v[4]), 0.0f });
        }
    }
#endif


    // == init renderpass ==
    // NOTE : 랜더링하는 순서대로 추가 할 것
    shadowPass = std::make_unique<ShadowPass>();
    geometryPass = std::make_unique<GeometryPass>();
    decalPass = std::make_unique<DecalPass>();
    lightPass = std::make_unique<LightPass>();
    skyboxPass = std::make_unique<SkyboxPass>();
    forwardTransparentPass = std::make_unique<ForwardTransparentPass>();
    bloomPass = std::make_unique<BloomPass>();
    postProcessPass = std::make_unique<PostProcessPass>();
    frustumPass = std::make_unique<FrustumPass>();
    uiPass = std::make_unique<UIRenderPass>();
    effectPass = std::make_unique<EffectPass>();

    shadowPass->Init();
    geometryPass->Init();
    decalPass->Init(dxRenderer->GetDevice());
    lightPass->Init(dxRenderer->GetDevice());
    skyboxPass->Init(dxRenderer->GetDevice());
    forwardTransparentPass->Init();
    bloomPass->Init();
    postProcessPass->Init();
    frustumPass->Init(dxRenderer->GetDevice(), dxRenderer->GetDeviceContext());
    uiPass->Init(dxRenderer->GetDevice());
    effectPass->Init(dxRenderer->GetDevice());

    // == init world data ==
    //WorldManager::Instance().shaderResourceView = shadowPass->GetShadowSRV();



#if _DEBUG
    // GPU Profiler는 실행 인자로 켤 때만 동작 (평소 에디터/게임에는 영향 없음)
    //  --gpu-profiler : Profiler 창 + 라이팅 모드 수동 전환
    //  --gpu-bench    : 세 모드 자동 측정 -> WorkSpace/gpu_bench.txt 저장 후 종료
    gpuBench = wcsstr(GetCommandLineW(), L"--gpu-bench") != nullptr;
    gpuProfilerEnabled = gpuBench || wcsstr(GetCommandLineW(), L"--gpu-profiler") != nullptr;
    if (gpuBench) lightingMode = LightingMode::ForwardOnly;
#else
    PlayModeSystem::Instance().SetPlayMode(PlayModeState::Playing);
#endif

    return true;
}

void EngineApp::OnPreUpdate()
{
    if (SceneSystem::Instance().isSceneChanging) SceneSystem::Instance().isSceneChanging = false; // NOTE : 플래그 활성화 되어있으면 비활성화 하기

    ScriptSystem::Instance().CheckReadyQueue();
    RenderSystem::Instance().CheckReadyQueue();
}

void EngineApp::OnUpdate()
{
    Camera* curCam;
    if (PlayModeSystem::Instance().IsPlaying())
        curCam = CameraSystem::Instance().GetCurrCamera();
    else
        curCam = CameraSystem::Instance().GetFreeCamera();

    SceneSystem::Instance().BeforUpdate();
    CameraSystem::Instance().FreeCameraUpdate(GameTimer::Instance().DeltaTime());
    CameraSystem::Instance().LightCameraUpdate(GameTimer::Instance().DeltaTime());
    WorldManager::Instance().Update(dxRenderer->GetDeviceContext(), curCam, clientWidth, clientHeight);
    SceneSystem::Instance().UpdateScene(GameTimer::Instance().DeltaTime());
    EffectSystem::Instance().Update();
    AudioManager::Instance().Update();
    AnimationSystem::Instance().Update(GameTimer::Instance().DeltaTime());

#if _DEBUG
    editor->Update();
#endif
}

void EngineApp::OnRender()
{
    BeginRender(); 					// 업데이트 준비

    RenderSystem::Instance().Render(*renderQueue);  // 렌더 컴포넌트

    // Default CB Setting
    {
        auto& sm = ShaderManager::Instance();
        const auto& context = dxRenderer->GetDeviceContext();

        // CB Slot Binding
        context->VSSetConstantBuffers(0, 1, sm.frameCB.GetAddressOf());
        context->VSSetConstantBuffers(1, 1, sm.transformCB.GetAddressOf());
        context->VSSetConstantBuffers(2, 1, sm.lightingCB.GetAddressOf());
        context->VSSetConstantBuffers(3, 1, sm.materialCB.GetAddressOf());
        context->VSSetConstantBuffers(4, 1, sm.offsetMatrixCB.GetAddressOf());
        context->VSSetConstantBuffers(5, 1, sm.poseMatrixCB.GetAddressOf());
        context->VSSetConstantBuffers(6, 1, sm.postProcessCB.GetAddressOf());
        context->VSSetConstantBuffers(7, 1, sm.bloomCB.GetAddressOf());
        context->VSSetConstantBuffers(8, 1, sm.effectCB.GetAddressOf());
        context->VSSetConstantBuffers(10, 1, sm.decalCB.GetAddressOf());

        context->PSSetConstantBuffers(0, 1, sm.frameCB.GetAddressOf());
        context->PSSetConstantBuffers(1, 1, sm.transformCB.GetAddressOf());
        context->PSSetConstantBuffers(2, 1, sm.lightingCB.GetAddressOf());
        context->PSSetConstantBuffers(3, 1, sm.materialCB.GetAddressOf());
        context->PSSetConstantBuffers(4, 1, sm.offsetMatrixCB.GetAddressOf());
        context->PSSetConstantBuffers(5, 1, sm.poseMatrixCB.GetAddressOf());
        context->PSSetConstantBuffers(6, 1, sm.postProcessCB.GetAddressOf());
        context->PSSetConstantBuffers(7, 1, sm.bloomCB.GetAddressOf());
        context->PSSetConstantBuffers(8, 1, sm.effectCB.GetAddressOf());
        context->PSSetConstantBuffers(10, 1, sm.decalCB.GetAddressOf());
    }

    // current camera
    Camera* curCam;
    if (PlayModeSystem::Instance().IsPlaying())
        curCam = CameraSystem::Instance().GetCurrCamera();
    else
        curCam = CameraSystem::Instance().GetFreeCamera();

    // NOTE : 렌더 패스 처리하기전에 씬을 교체하는지 확인하고 교체하면 큐 클리어
    if(SceneSystem::Instance().isSceneChanging)
    {
        renderQueue->Clear();
    }

    // lighting mode (GPU Profiler 비교용, 기본값은 DeferredLightVolume)
    const bool forwardOnly = lightingMode == LightingMode::ForwardOnly;
    lightPass->forceFullScreen = lightingMode == LightingMode::DeferredFullScreen;
    forwardTransparentPass->drawOpaque = forwardOnly;

    // render pass
#if _DEBUG
    GpuProfileBegin();
#endif
    RunPass(*shadowPass, GP_Shadow, curCam);
    if (forwardOnly)
    {
        // Deferred 패스를 건너뛰므로 LightPass가 하던 sceneHDR clear를 대신 한다
        const float black[4] = { 0, 0, 0, 1 };
        dxRenderer->GetDeviceContext()->ClearRenderTargetView(ShaderManager::Instance().sceneHDRRTV.Get(), black);
    }
    else
    {
        RunPass(*geometryPass, GP_Geometry, curCam);
        RunPass(*decalPass, GP_Decal, curCam);
        RunPass(*lightPass, GP_Light, curCam);
    }
    RunPass(*skyboxPass, GP_Skybox, curCam);
    RunPass(*forwardTransparentPass, GP_Forward, curCam);
    RunPass(*effectPass, GP_Effect, curCam);    // note : 26.02.09 파티클 패스
    RunPass(*bloomPass, GP_Bloom, curCam);
    RunPass(*postProcessPass, GP_PostProcess, curCam);
    RunPass(*uiPass, GP_UI, curCam);
#if _DEBUG
    GpuProfileEnd();
#endif


#if _DEBUG
    editor->Render(hwnd); 	// 엔진 오버레이 렌더링
    RenderGpuProfiler();
    if (benchExitFrames > 0 && --benchExitFrames == 0) PostMessage(hwnd, WM_CLOSE, 0, 0);
    imguiRenderer->Render();		// imgui 렌더링
    editor->RenderEnd(dxRenderer->GetDeviceContext());
#endif

    EndRender(); 					// 업데이트 마무리
}

void EngineApp::RunPass(IRenderPass& pass, GpuPass id, Camera* cam)
{
    dxRenderer->ProcessScene(*renderQueue, pass, cam);

#if _DEBUG
    // 패스가 끝난 시점의 GPU timestamp (이전 timestamp와의 차이 = 이 패스의 GPU 시간)
    auto& q = gpuQueries[gpuFrameIndex % kQueryLatency];
    if (gpuProfilerEnabled && q.count < GP_Count && q.stamps[q.count])
    {
        q.ids[q.count] = id;
        dxRenderer->GetDeviceContext()->End(q.stamps[q.count].Get());
        q.count++;
    }
#endif
}

#if _DEBUG
void EngineApp::GpuProfileBegin()
{
    if (!gpuProfilerEnabled) return;
    auto device = dxRenderer->GetDevice();
    auto context = dxRenderer->GetDeviceContext();
    auto& q = gpuQueries[gpuFrameIndex % kQueryLatency];

    if (!q.disjoint)
    {
        D3D11_QUERY_DESC desc = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
        HR_T(device->CreateQuery(&desc, q.disjoint.GetAddressOf()));
        desc.Query = D3D11_QUERY_TIMESTAMP;
        HR_T(device->CreateQuery(&desc, q.start.GetAddressOf()));
        for (auto& stamp : q.stamps)
            HR_T(device->CreateQuery(&desc, stamp.GetAddressOf()));
    }

    q.count = 0;
    q.mode = lightingMode;
    context->Begin(q.disjoint.Get());
    context->End(q.start.Get());
}

void EngineApp::GpuProfileEnd()
{
    if (!gpuProfilerEnabled) return;
    auto context = dxRenderer->GetDeviceContext();
    context->End(gpuQueries[gpuFrameIndex % kQueryLatency].disjoint.Get());
    gpuQueries[gpuFrameIndex % kQueryLatency].issued = true;
    gpuFrameIndex++;

    // 가장 오래된 (kQueryLatency - 1 프레임 전) 결과 읽기. 다음 프레임에 이 슬롯을 다시 쓴다.
    auto& r = gpuQueries[gpuFrameIndex % kQueryLatency];
    if (!r.issued) return;
    r.issued = false;

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT dj = {};
    if (context->GetData(r.disjoint.Get(), &dj, sizeof(dj), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK) return;
    if (dj.Disjoint || dj.Frequency == 0) return;   // GPU 클럭이 바뀐 프레임은 버림

    UINT64 prev = 0;
    if (context->GetData(r.start.Get(), &prev, sizeof(prev), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK) return;
    const UINT64 first = prev;

    double passMs[GP_Count] = {};
    bool used[GP_Count] = {};
    for (int i = 0; i < r.count; i++)
    {
        UINT64 t = 0;
        if (context->GetData(r.stamps[i].Get(), &t, sizeof(t), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK) return;
        passMs[r.ids[i]] = double(t - prev) * 1000.0 / double(dj.Frequency);
        used[r.ids[i]] = true;
        prev = t;
    }

    // 모드가 바뀌면 누적을 새로 시작 (모드가 섞인 평균 방지)
    if (r.mode != accumMode)
    {
        accumMode = r.mode;
        accumFrames = 0;
        accumFrameMs = 0.0;
        for (int i = 0; i < GP_Count; i++) { accumPassMs[i] = 0.0; accumPassUsed[i] = false; }
    }

    for (int i = 0; i < GP_Count; i++)
    {
        accumPassMs[i] += passMs[i];
        accumPassUsed[i] |= used[i];
    }
    accumFrameMs += double(prev - first) * 1000.0 / double(dj.Frequency);
    accumFrames++;

    if (accumFrames >= kAverageFrames)
    {
        auto& res = gpuResults[(int)accumMode];
        res.valid = true;
        res.frameMs = float(accumFrameMs / accumFrames);
        for (int i = 0; i < GP_Count; i++)
        {
            res.passMs[i] = float(accumPassMs[i] / accumFrames);
            res.passUsed[i] = accumPassUsed[i];
            accumPassMs[i] = 0.0;
            accumPassUsed[i] = false;
        }
        // Forward Only는 LightPass를 건너뛰므로 카운트가 없음
        const bool deferred = accumMode != LightingMode::ForwardOnly;
        res.volumeLights = deferred ? lightPass->volumeLightCount : 0;
        res.fullScreenLights = deferred ? lightPass->fullScreenLightCount : 0;
        accumFrameMs = 0.0;
        accumFrames = 0;

        if (gpuBench) GpuBenchStep();
    }
}

void EngineApp::GpuBenchStep()
{
    if (benchExitFrames >= 0) return;
    if (++benchPublishes < 2) return;   // 모드 전환 직후 평균은 버리고 두 번째 평균을 사용

    benchPublishes = 0;
    int next = (int)lightingMode + 1;
    if (next < (int)LightingMode::Count)
    {
        lightingMode = (LightingMode)next;
        return;
    }

    GpuBenchWrite();
    benchExitFrames = 300;  // 결과 창을 캡처할 시간
}

void EngineApp::GpuBenchWrite()
{
    static const char* modeNames[] = { "ForwardOnly", "DeferredFullScreen", "DeferredLightVolume" };
    static const char* passNames[] = { "Shadow", "Geometry", "Decal", "Light", "Skybox",
                                       "Forward", "Effect", "Bloom", "PostProcess", "UI" };
    const auto& vp = ShaderManager::Instance().viewport_screen;

    FILE* fp = nullptr;
    if (fopen_s(&fp, "gpu_bench.txt", "w") != 0 || !fp) return;
    fprintf(fp, "resolution=%.0fx%.0f opaque=%d transparent=%d lights=%d shaderOptimized=%d averageFrames=%d\n",
        vp.Width, vp.Height, (int)renderQueue->GetOpaqueQueue().size(),
        (int)renderQueue->GetTransparentQueue().size(),
        (int)LightSystem::Instance().GetComponents().size(), 1, kAverageFrames);  // --gpu-bench는 항상 셰이더 최적화
    fprintf(fp, "pass");
    for (auto* m : modeNames) fprintf(fp, ",%s", m);
    fprintf(fp, "\nFrame");
    for (auto& r : gpuResults) fprintf(fp, ",%.3f", r.frameMs);
    for (int p = 0; p < GP_Count; p++)
    {
        fprintf(fp, "\n%s", passNames[p]);
        for (auto& r : gpuResults)
            r.passUsed[p] ? fprintf(fp, ",%.3f", r.passMs[p]) : fprintf(fp, ",skip");
    }
    fprintf(fp, "\nVolumeLights");
    for (auto& r : gpuResults) fprintf(fp, ",%d", r.volumeLights);
    fprintf(fp, "\nFullScreenLights");
    for (auto& r : gpuResults) fprintf(fp, ",%d", r.fullScreenLights);
    fprintf(fp, "\n");
    fclose(fp);
}

void EngineApp::RenderGpuProfiler()
{
    if (!gpuProfilerEnabled) return;

    static const char* modeNames[] = { "Forward Only", "Deferred (FullScreen)", "Deferred + LightVolume" };
    static const char* passNames[] = { "Shadow", "Geometry", "Decal", "Light", "Skybox",
                                       "Forward", "Effect", "Bloom", "PostProcess", "UI" };
    static_assert(sizeof(passNames) / sizeof(passNames[0]) == GP_Count, "pass name count");

    ImGui::Begin("GPU Profiler");

    int mode = (int)lightingMode;
    ImGui::Text("Lighting Mode");
    for (int i = 0; i < (int)LightingMode::Count; i++)
    {
        ImGui::SameLine();
        ImGui::RadioButton(modeNames[i], &mode, i);
    }
    lightingMode = (LightingMode)mode;

    const auto& vp = ShaderManager::Instance().viewport_screen;
    ImGui::Text("Resolution : %.0f x %.0f   Opaque : %d   Transparent : %d   Lights : %d",
        vp.Width, vp.Height,
        (int)renderQueue->GetOpaqueQueue().size(), (int)renderQueue->GetTransparentQueue().size(),
        (int)LightSystem::Instance().GetComponents().size());
    ImGui::TextDisabled("%d frame average. Switch mode and wait a few seconds.", kAverageFrames);
    ImGui::TextColored(ImVec4(0.4f, 1, 0.4f, 1), "Shader : optimized (--gpu-profiler / --gpu-bench)");

    if (ImGui::BeginTable("gpu", 1 + (int)LightingMode::Count, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("ms");
        for (auto* name : modeNames) ImGui::TableSetupColumn(name);
        ImGui::TableHeadersRow();

        auto row = [&](const char* label, auto cell)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(label);
            for (int m = 0; m < (int)LightingMode::Count; m++)
            {
                ImGui::TableNextColumn();
                if (gpuResults[m].valid) cell(gpuResults[m]);
                else ImGui::TextDisabled("-");
            }
        };

        row("GPU Frame Time", [](const GpuModeResult& r) { ImGui::Text("%.3f", r.frameMs); });
        for (int p = 0; p < GP_Count; p++)
        {
            row(passNames[p], [p](const GpuModeResult& r)
            {
                if (r.passUsed[p]) ImGui::Text("%.3f", r.passMs[p]);
                else ImGui::TextDisabled("skip");
            });
        }
        row("Point/Spot : Volume", [](const GpuModeResult& r) { ImGui::Text("%d", r.volumeLights); });
        row("Point/Spot : FullScreen", [](const GpuModeResult& r) { ImGui::Text("%d", r.fullScreenLights); });

        ImGui::EndTable();
    }

    ImGui::End();
}
#endif

void EngineApp::OnFixedUpdate()
{
    // [ Physics Update : 프레임 드랍 방지 ] 
    constexpr float fixedDt = 1.0f / 60.0f;
    if (GameTimer::Instance().DeltaTime() > 0.1f)
        m_PhysicsAccumulator += 0.1f;
    else
        m_PhysicsAccumulator += GameTimer::Instance().DeltaTime();

    while (m_PhysicsAccumulator >= fixedDt)
    {
        SceneSystem::Instance().FixedUpdateScene(fixedDt);

        if (PlayModeSystem::Instance().IsPlaying())
        {
            PhysicsSystem::Instance().Simulate(fixedDt);
            AgentSystem::Instance().FixedUpdate(fixedDt);
        }
        GridSystem::Instance().FixedUpdate(fixedDt);

        m_PhysicsAccumulator -= fixedDt;
    }
}

void EngineApp::OnLateUpdate()
{
    SceneSystem::Instance().LateUpdateScene(GameTimer::Instance().DeltaTime());
}

void GameApp::ConsoleInitialize()
{
#if _DEBUG
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    SetConsoleTitle(L"윈도우 메세지 콘솔 로그");
    printf("-- Console log start --\n\n");
#endif
}

void GameApp::ConsoleUninitalize()
{
#if _DEBUG
    // 표준 출력 스트림 닫기
    fclose(stdout);
    // 콘솔 해제
    FreeConsole();
#endif
}

void EngineApp::BeginRender()
{
#if _DEBUG
    imguiRenderer->BeginRender();
#endif
    renderer->BeginRender();
    renderQueue->Clear();
}

void EngineApp::EndRender()
{
    renderer->EndRender();
#if _DEBUG
    imguiRenderer->EndRender();
#endif
}

#include "Util/PathHelper.h"

void EngineApp::LoadSavedFirstScene()
{
    auto sceneDir = PathHelper::FindDirectory("Assets/Scenes");
    if (!sceneDir) // Assets/Scenes가 존재하지 않음
    {
        throw std::runtime_error("Failed find scene directory");
    }

    fs::path sceneFile;

#if _DEBUG
    // --scene=04_MainGameScene.json : 지정한 씬으로 시작 (GPU 벤치마크용)
    std::wstring cmd = GetCommandLineW();
    if (auto pos = cmd.find(L"--scene="); pos != std::wstring::npos)
    {
        std::wstring name = cmd.substr(pos + 8);
        sceneFile = *sceneDir / name.substr(0, name.find(L' '));
    }
#endif

    if (sceneFile.empty())
    for (const auto& entry : fs::directory_iterator(*sceneDir))
    {
        if (!entry.is_regular_file()) continue; // 일반 파일일지 설정하기

        if (entry.path().extension() == ".json")
        {
            sceneFile = entry.path();
            break;
        }
    }

    // 폴더에 씬이 없으면 그냥 시작
    // 있으면 맨 첫번째 파일을 읽어서 씬 구성
    if (!sceneFile.empty())
    {
        auto currScene = SceneSystem::Instance().GetCurrentScene();
        if (!currScene) throw std::runtime_error("Faild to get currentScene, SceneSystem is not initalized");
        currScene->LoadToJson(sceneFile.string());
    }
}

void EngineApp::ResizeScreen(int width, int height)
{
    if (!hwnd) return;

    clientWidth = max(width, 1);
    clientHeight = max(height, 1);

    ResizeResource();
}

void EngineApp::ResizeResource()
{
    // clear
    auto& sm = ShaderManager::Instance();

    sm.ReleaseBackBufferResources();
#if _DEBUG
    editor->ReleaseBackBufferResources();
#endif

    dxRenderer->OnResize(clientWidth, clientHeight);
    sm.CreateBackBufferResource(dxRenderer->GetDevice(), clientWidth, clientHeight);

    // shadowManager 초기화
    sm.backBufferRTV = dxRenderer->GetBackBufferRTV();
    sm.depthStencilView = dxRenderer->GetDepthStencilView();
    sm.depthStencilReadOnlyView = dxRenderer->GetDepthStencilReadOnlyView();
    sm.depthSRV = dxRenderer->GetDepthSRV();
    sm.viewport_screen = dxRenderer->GetRenderViewPort();

    UIManager::Instance().SetSize(clientWidth, clientHeight);

#if _DEBUG
    // editor 참조
    editor->GetScreenSize(clientWidth, clientHeight);
    editor->GetDSV(dxRenderer->GetDepthStencilView());
    editor->GetRTV(dxRenderer->GetBackBufferRTV());
    editor->CreatePickingStagingTex();
#endif

    // Camera
    auto cams = CameraSystem::Instance().GetAllCamera();
    for (auto& e : cams)
    {
        e->SetProjection(e->GetPovAngle(), clientWidth, clientHeight, e->GetNearDist(), e->GetFarDist());
    }
}

// Forward declare message handler from imgui_impl_win32.cpp
#if _DEBUG
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

LRESULT EngineApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#if _DEBUG
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;
#endif

    switch (message)
    {
    //case WM_ENTERSIZEMOVE:
    //    isResize = true;
    //    break;
    //case WM_EXITSIZEMOVE:
    //    isResize = false;
    //    RECT rc;
    //    GetClientRect(hWnd, &rc);
    //
    //    if (rc.right - rc.left == clientWidth && rc.bottom - rc.top == clientHeight) break;
    //
    //    ResizeScreen(rc.right - rc.left, rc.bottom - rc.top);
    //
    //    break;
    default:
        break;
    }

    return __super::WndProc(hWnd, message, wParam, lParam);
}

void EngineApp::OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker, const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker)
{
    __super::OnInputProcess(KeyState, KeyTracker, MouseState, MouseTracker);
#if _DEBUG
    editor->OnInputProcess(KeyState, KeyTracker, MouseState, MouseTracker);
#endif
}

// ================= 컴포넌트 등록 =================

#include "Components/FBXData.h"
#include "Components/FBXRenderer.h"
#include "Components/Transform.h"
#include "Components/Camera.h"
#include "Components/AudioListenerComponent.h"
#include "Components/AudioSourceComponent.h"
#include "Components/PhysicsComponent.h"
#include "Components/CharacterControllerComponent.h"
#include "Components/AnimationController.h"
#include "Components/GridComponent.h"
#include "Components/AgentComponent.h"
#include "Components/VisionComponent.h"
#include "Manager/ComponentFactory.h"

#include "99_Test/Player/Player1.h"
#include "99_Test/Player/Weapon.h"


#include "99_Test/PhysicsTest/PhysicsTestScript.h"
#include "99_Test/PhysicsTest/GroundTestScript.h"
#include "99_Test/PhysicsTest/CCTTest.h"

#include "Components/UI/Image.h"
#include "Components/RectTransform.h"
#include "Components/UI/TextUI.h"
#include "RenderPass/ParticleSource/Effect.h"

void EngineApp::RegisterAllComponents()
{
    auto& cf = ComponentFactory::Instance();

    cf.Register<Transform>("Transform", ComponentCategory::Core);
    cf.Register<Camera>("Camera", ComponentCategory::Core);

    cf.Register<FBXData>("FBXData", ComponentCategory::Rendering);
    cf.Register<FBXRenderer>("FBXRenderer", ComponentCategory::Rendering);
    cf.Register<Light>("Light", ComponentCategory::Rendering);
    cf.Register<Decal>("Decal", ComponentCategory::Rendering);

    cf.Register<AudioListenerComponent>("AudioListenerComponent", ComponentCategory::Audio);
    cf.Register<AudioSourceComponent>("AudioSourceComponent", ComponentCategory::Audio);

    cf.Register<PhysicsComponent>("PhysicsComponent", ComponentCategory::Physics);
    cf.Register<CharacterControllerComponent>("CharacterControllerComponent", ComponentCategory::Physics);
    cf.Register<GridComponent>("GridComponent", ComponentCategory::Physics);
    cf.Register<AgentComponent>("AgentComponent", ComponentCategory::Physics);
    cf.Register<VisionComponent>("VisionComponent", ComponentCategory::Physics);

    cf.Register<AnimationController>("AnimationController", ComponentCategory::Animation);

    cf.Register<PhysicsTestScript>("PhysicsTestScript", ComponentCategory::Script);
    cf.Register<GroundTestScript>("GroundTestScript", ComponentCategory::Script);
    cf.Register<CCTTest>("CCTTestScript", ComponentCategory::Script);

    cf.Register<RectTransform>("RectTransform", ComponentCategory::Other);
    cf.Register<Image>("Image", ComponentCategory::UI);

    Woo_Registeration();
    Moon_Registeration();
    Ron_Registeration();
    Ho_Registeration();
}

void EngineApp::Woo_Registeration()
{
}

void EngineApp::Moon_Registeration()
{
}

void EngineApp::Ron_Registeration()
{

}

void EngineApp::Ho_Registeration()
{
    auto& cf = ComponentFactory::Instance();

    cf.Register<TextUI>("TextUI", ComponentCategory::UI);
    cf.Register<Effect>("Effect", ComponentCategory::Other);
}
