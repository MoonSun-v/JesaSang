#if _DEBUG
#include "imgui_impl_win32.h" // ImGui_ImplWin32_WndProcHandler 사용하기 위함
#endif

#include "GameApp.h"
#include "EngineApp.h"


#include "Manager/FBXResourceManager.h"
#include "Manager/ShaderManager.h"
#include "Manager/WorldManager.h"

#include "Entity/Object.h"
#include "Object/GameObject.h"

#include "System/ObjectSystem.h"
#include "EngineSystem/CameraSystem.h"
#include "EngineSystem/PlayModeSystem.h"
#include "EngineSystem/LightSystem.h"

#include "Components/FreeCamera.h"

namespace fs = std::filesystem;

EngineApp::EngineApp(HINSTANCE hInstance)
	: GameApp(hInstance)
{
}

EngineApp::~EngineApp()
{
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

    auto& sm = ShaderManager::Instance();
    sm.viewport_screen = dxRenderer->GetRenderViewPort();
    sm.backBufferRTV = dxRenderer->GetBackBufferRTV();
    sm.depthStencilView = dxRenderer->GetDepthStencilView();
    sm.depthStencilReadOnlyView = dxRenderer->GetDepthStencilReadOnlyView();
    sm.depthSRV = dxRenderer->GetDepthSRV();


    renderQueue = std::make_unique<RenderQueue>();

#if _DEBUG
	editor = std::make_unique<Editor>();
	editor->GetScreenSize(clientWidth, clientHeight);
	editor->Initialize(dxRenderer->GetDevice(), dxRenderer->GetDeviceContext());
	editor->GetDSV(dxRenderer->GetDepthStencilView());
	editor->GetRTV(dxRenderer->GetBackBufferRTV());
#endif

	SceneSystem::Instance().AddScene();			    	// create first scene
	SceneSystem::Instance().SetCurrentSceneByIndex(); 	// render first scene

	// create free camera
	CameraSystem::Instance().SetScreenSize(clientWidth, clientHeight);

#if _DEBUG
	auto freeCamHandle = CameraSystem::Instance().CreateFreeCamera(clientWidth, clientHeight, SceneSystem::Instance().GetCurrentScene().get());
    auto freeCamObjPtr = ObjectSystem::Instance().Get<GameObject>(freeCamHandle);
    freeCamObjPtr->AddComponent<FreeCamera>();
#endif

    // == find scene ==
    LoadSavedFirstScene();


	// == init renderpass ==
	// NOTE : 랜더링하는 순서대로 추가 할 것
    shadowPass = std::make_unique<ShadowPass>();
    geometryPass = std::make_unique<GeometryPass>();
    lightPass = std::make_unique<LightPass>();
    skyboxPass = std::make_unique<SkyboxPass>();
    bloomPass = std::make_unique<BloomPass>();
    postProcessPass = std::make_unique<PostProcessPass>();

    shadowPass->Init();
    geometryPass->Init();
    lightPass->Init(dxRenderer->GetDevice());
    skyboxPass->Init(dxRenderer->GetDevice());
    bloomPass->Init();
    postProcessPass->Init();


    // == init world data ==
	//WorldManager::Instance().shaderResourceView = shadowPass->GetShadowSRV();

 

#if _DEBUG
#else
    PlayModeSystem::Instance().SetPlayMode(PlayModeState::Playing);
#endif

	return true;
}

void EngineApp::OnUpdate()
{
    auto freeCam = CameraSystem::Instance().GetFreeCamera();
    auto currCam = CameraSystem::Instance().GetCurrCamera();

	SceneSystem::Instance().BeforUpdate();	
	CameraSystem::Instance().FreeCameraUpdate(GameTimer::Instance().DeltaTime());
	CameraSystem::Instance().LightCameraUpdate(GameTimer::Instance().DeltaTime());
	WorldManager::Instance().Update(dxRenderer->GetDeviceContext(), freeCam, clientWidth, clientHeight);
	SceneSystem::Instance().UpdateScene(GameTimer::Instance().DeltaTime());

#if _DEBUG
	editor->Update();
#endif
}

void EngineApp::OnRender()
{
	BeginRender(); 					// 업데이트 준비

    RenderSystem::Instance().Render(*renderQueue);
	
	auto freeCam = CameraSystem::Instance().GetFreeCamera();	
    auto currCam = CameraSystem::Instance().GetCurrCamera();

    // Default CB Setting (TODO :: 이거 어디로 뺄까?)
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

        context->PSSetConstantBuffers(0, 1, sm.frameCB.GetAddressOf());
        context->PSSetConstantBuffers(1, 1, sm.transformCB.GetAddressOf());
        context->PSSetConstantBuffers(2, 1, sm.lightingCB.GetAddressOf());
        context->PSSetConstantBuffers(3, 1, sm.materialCB.GetAddressOf());
        context->PSSetConstantBuffers(4, 1, sm.offsetMatrixCB.GetAddressOf());
        context->PSSetConstantBuffers(5, 1, sm.poseMatrixCB.GetAddressOf());
        context->PSSetConstantBuffers(6, 1, sm.postProcessCB.GetAddressOf());
        context->PSSetConstantBuffers(7, 1, sm.bloomCB.GetAddressOf());
        context->PSSetConstantBuffers(8, 1, sm.effectCB.GetAddressOf());
    }

    if (PlayModeSystem::Instance().IsPlaying())
    {
        dxRenderer->ProcessScene(*renderQueue, *shadowPass, currCam);
        dxRenderer->ProcessScene(*renderQueue, *geometryPass, currCam);
        dxRenderer->ProcessScene(*renderQueue, *lightPass, currCam);
        dxRenderer->ProcessScene(*renderQueue, *skyboxPass, currCam);
        dxRenderer->ProcessScene(*renderQueue, *bloomPass, currCam);
        dxRenderer->ProcessScene(*renderQueue, *postProcessPass, currCam);
    }
    else
    {
        dxRenderer->ProcessScene(*renderQueue, *shadowPass, freeCam);
        dxRenderer->ProcessScene(*renderQueue, *geometryPass, freeCam);
        dxRenderer->ProcessScene(*renderQueue, *lightPass, freeCam);
        dxRenderer->ProcessScene(*renderQueue, *skyboxPass, freeCam);
        dxRenderer->ProcessScene(*renderQueue, *bloomPass, freeCam);
        dxRenderer->ProcessScene(*renderQueue, *postProcessPass, freeCam);
    }

#if _DEBUG
	editor->Render(hwnd); 	// 엔진 오버레이 렌더링
	imguiRenderer->Render();		// imgui 렌더링
	editor->RenderEnd(dxRenderer->GetDeviceContext());
#endif

	EndRender(); 					// 업데이트 마무리
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
    editor->ReleaseBackBufferResources();

    dxRenderer->OnResize(clientWidth, clientHeight);
    sm.CreateBackBufferResource(dxRenderer->GetDevice(), clientWidth, clientHeight);

    // shadowManager 초기화
    sm.backBufferRTV = dxRenderer->GetBackBufferRTV();
    sm.depthStencilView = dxRenderer->GetDepthStencilView();
    sm.depthStencilReadOnlyView = dxRenderer->GetDepthStencilReadOnlyView();
    sm.depthSRV = dxRenderer->GetDepthSRV();
    sm.viewport_screen = dxRenderer->GetRenderViewPort();

    // editor 참조
    editor->GetDSV(dxRenderer->GetDepthStencilView());
    editor->GetRTV(dxRenderer->GetBackBufferRTV());

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
    case WM_ENTERSIZEMOVE:
        isResize = true;
        break;
    case WM_EXITSIZEMOVE:
        isResize = false;
        RECT rc;
        GetClientRect(hWnd, &rc);

        if (rc.right - rc.left == clientWidth && rc.bottom - rc.top == clientHeight) break;

        ResizeScreen(rc.right - rc.left, rc.bottom - rc.top);

        break;
    default:
        break;
    }

	return __super::WndProc(hWnd, message, wParam, lParam);
}

void EngineApp::OnInputProcess(const Keyboard::State &KeyState, const Keyboard::KeyboardStateTracker &KeyTracker, const Mouse::State &MouseState, const Mouse::ButtonStateTracker &MouseTracker)
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
#include "Manager/ComponentFactory.h"

#include "Player/Player1.h"
#include "Player/Weapon.h"

void EngineApp::RegisterAllComponents()
{
	ComponentFactory::Instance().Register<FBXData>("FBXData");
	ComponentFactory::Instance().Register<FBXRenderer>("FBXRenderer");
	ComponentFactory::Instance().Register<Transform>("Transform");
	ComponentFactory::Instance().Register<Camera>("Camera");

	ComponentFactory::Instance().Register<Player1>("Player1");
	ComponentFactory::Instance().Register<Weapon>("Weapon");
	ComponentFactory::Instance().Register<Light>("Light");
}