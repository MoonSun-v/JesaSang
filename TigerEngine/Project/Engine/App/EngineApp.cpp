#if _DEBUG
#include "imgui_impl_win32.h" // ImGui_ImplWin32_WndProcHandler 사용하기 위함
#endif

#include "GameApp.h"
#include "GameApp.h"
#include "EngineApp.h"


#include "../Manager/FBXResourceManager.h"
#include "../Manager/ShaderManager.h"
#include "../Manager/WorldManager.h"

#include "../RenderPass/DirectionalLightPass.h"
#include "../RenderPass/GBufferRenderPass.h"
#include "../RenderPass/SkyboxRenderPass.h"
#include "../RenderPass/ShadowRenderPass.h"
#include "../RenderPass/DebugDrawPass.h"

#include "Entity/Object.h"
#include "Entity/GameObject.h"

#include "System/CameraSystem.h"
#include "System/ObjectSystem.h"

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
	ShaderManager::Instance().CreateCB(dxRenderer->GetDevice());

#if _DEBUG
	editor = std::make_unique<Editor>();
	editor->GetScreenSize(clientWidth, clientHeight);
	editor->Initialize(dxRenderer->GetDevice(), dxRenderer->GetDeviceContext());
	editor->GetDSV(dxRenderer->GetDepthStencilView());
	editor->GetRTV(dxRenderer->GetBackBufferRTV());
#endif

	SceneSystem::Instance().AddScene();				// create first scene
	SceneSystem::Instance().SetCurrentSceneByIndex(); 	// render first scene

	// == create free camera ==
	CameraSystem::Instance().CreateFreeCamera(clientWidth, clientHeight, SceneSystem::Instance().GetCurrentScene().get());
	WorldManager::Instance().CreateDirectionalLightFrustum(); // create directional light 


	// == init renderpass ==
	// NOTE : 랜더링하는 순서대로 추가 할 것
	auto shadowPass = std::make_shared<ShadowRenderPass>();
	shadowPass->Init(dxRenderer->GetDevice(), dxRenderer->GetDeviceContext(), CameraSystem::Instance().GetFreeCamera());
	renderPasses.push_back(shadowPass);

	WorldManager::Instance().shaderResourceView = shadowPass->GetShadowSRV();

	auto gpass = std::make_shared<GBufferRenderPass>();
	gpass->Init(dxRenderer->GetDevice(), dxRenderer->GetDeviceContext(), clientWidth, clientHeight);
	gpass->SetDepthStencilView(dxRenderer->GetDepthStencilView());
	renderPasses.push_back(gpass);

	auto dlpass = std::make_shared<DirectionalLightPass>();
	dlpass->Init(dxRenderer->GetDevice());
	dlpass->SetGBufferSRV(gpass->GetShaderResourceViews());
	dlpass->SetDepthStencilView(dxRenderer->GetDepthStencilView());
	dlpass->SetRenderTargetView(dxRenderer->GetBackBufferRTV());
	dlpass->SetShadowSRV(shadowPass->GetShadowSRV());
	renderPasses.push_back(dlpass);

	auto sbpass = std::make_shared<SkyboxRenderPass>();
	sbpass->Init(dxRenderer->GetDevice(), dxRenderer->GetDeviceContext(), clientWidth, clientHeight);
	sbpass->SetDepthStencilView(dxRenderer->GetDepthStencilView());
	sbpass->SetRenderTargetView(dxRenderer->GetBackBufferRTV());
	renderPasses.push_back(sbpass);

    // == find scene ==

    LoadSavedFirstScene();

	return true;
}

void EngineApp::OnUpdate()
{
	SceneSystem::Instance().BeforUpdate();
	
	// Scene의 오브젝트 업데이트 호출
	SceneSystem::Instance().UpdateScene(GameTimer::Instance().DeltaTime());

	CameraSystem::Instance().FreeCameraUpdate(GameTimer::Instance().DeltaTime());

	WorldManager::Instance().Update();

#if _DEBUG
	editor->Update();
#endif
}

void EngineApp::OnRender()
{
	BeginRender(); 					// 업데이트 준비
	
	auto freeCam = CameraSystem::Instance().GetFreeCamera();	

	for(auto& pass : renderPasses)
	{	
		if(typeid(*pass) == typeid(GBufferRenderPass))
		{
			dxRenderer->ProcessScene(SceneSystem::Instance().GetCurrentScene(), pass, freeCam);
		}
		else if(typeid(*pass) == typeid(ShadowRenderPass))
		{
			dxRenderer->ProcessScene(SceneSystem::Instance().GetCurrentScene(), pass, freeCam); 
		}
		else
		{
			dxRenderer->ProcessScene(nullptr, pass, freeCam);  // 렌더러가 씬을 렌더링
		}
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
}

void EngineApp::EndRender()
{	
	renderer->EndRender();
#if _DEBUG
	imguiRenderer->EndRender();
#endif
}

void EngineApp::LoadSavedFirstScene()
{
    auto sceneDir = FindDirectoryInAssets("Scenes");
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

std::optional<fs::path> EngineApp::FindDirectoryInAssets(std::string dirName)
{
    fs::path cur = GetExeDir(); // fx::current_path는 IDE와 환경에 따라 다를 수 있으므로 따로 이름 가져오기
    for (int i = 0; i < 5; i++)
    {
        fs::path candidate = cur /"Assets" / dirName; // 현재 폴더
        if (fs::exists(candidate) && fs::is_directory(candidate)) // 해당 위치에 assets/scene 폴더가 존재한다.
        {
            return candidate;
        }
        cur = cur.parent_path();
    }   

    std::string errorMessage = "failed find scenes directory\n Dir : module/Asssets/" + dirName;
    MessageBoxA(hwnd, errorMessage.c_str(), "Error", MB_OK);
    return std::nullopt;
}

std::filesystem::path EngineApp::GetExeDir()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
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

	return __super::WndProc(hWnd, message, wParam, lParam);
}

void EngineApp::OnInputProcess(const Keyboard::State &KeyState, const Keyboard::KeyboardStateTracker &KeyTracker, const Mouse::State &MouseState, const Mouse::ButtonStateTracker &MouseTracker)
{
	__super::OnInputProcess(KeyState, KeyTracker, MouseState, MouseTracker);
#if _DEBUG
	editor->OnInputProcess(KeyState, KeyTracker, MouseState, MouseTracker);
#endif
	InputSystem::Instance().UpdateRegisterInput(KeyState,KeyTracker, MouseState, MouseTracker);
}

// ================= 컴포넌트 등록 =================

#include "../Components/FBXData.h"
#include "../Components/FBXRenderer.h"
#include "Entity/Transform.h"
#include "Entity/Camera.h"
#include "System/ComponentFactory.h"

void EngineApp::RegisterAllComponents()
{
	ComponentFactory::Instance().Register<FBXData>("FBXData");
	ComponentFactory::Instance().Register<FBXRenderer>("FBXRenderer");
	ComponentFactory::Instance().Register<Transform>("Transform");
	ComponentFactory::Instance().Register<Transform>("Camera");
}