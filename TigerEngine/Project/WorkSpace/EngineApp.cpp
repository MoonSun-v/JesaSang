#if _DEBUG
#include "imgui_impl_win32.h" // ImGui_ImplWin32_WndProcHandler 사용하기 위함
#endif

#include "GameApp.h"
#include "EngineApp.h"


#include "Manager/FBXResourceManager.h"
#include "Manager/AudioManager.h"
#include "Manager/ShaderManager.h"
#include "Manager/WorldManager.h"

#include "RenderPass/DirectionalLightPass.h"
#include "RenderPass/GBufferRenderPass.h"
#include "RenderPass/SkyboxRenderPass.h"
#include "RenderPass/ShadowRenderPass.h"
#include "RenderPass/DebugDrawPass.h"

#include "Entity/Object.h"
#include "Entity/GameObject.h"

#include "System/CameraSystem.h"
#include "System/ObjectSystem.h"
#include "System/PlayModeSystem.h"

#include "Components/FreeCamera.h"
#include "AudioListenerComponent.h"
#include "AudioSourceComponent.h"
#include "AudioClip.h"

namespace fs = std::filesystem;

namespace
{
    AudioListenerComponent g_audioListener;
    AudioSourceComponent g_audioSource;
    std::shared_ptr<AudioClip> g_audioClip;

    DirectX::XMFLOAT3 g_listenerPos{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 g_listenerVel{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 g_listenerFwd{ 0.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT3 g_listenerUp{ 0.0f, 1.0f, 0.0f };

    DirectX::XMFLOAT3 g_sourcePos{ 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 g_sourceVel{ 0.0f, 0.0f, 0.0f };

    bool g_audioReady = false;
    bool g_audioStarted = false;

    void TrySetupAudioTest()
    {
        auto& audioMgr = AudioManager::Instance();
        auto entry = audioMgr.GetEntry("SFX_TestLoop");
        if (!entry)
        {
            return;
        }

        audioMgr.GetSystem().Set3DSettings(1.0f, 1.0f, 1.0f);

        g_audioClip = audioMgr.GetOrCreateClip(entry->id);
        if (!g_audioClip)
        {
            return;
        }

        g_audioListener.Init(&audioMgr.GetSystem());
        g_audioSource.Init(&audioMgr.GetSystem());

        AudioTransformRef listenerRef{};
        listenerRef.position = &g_listenerPos;
        listenerRef.velocity = &g_listenerVel;
        listenerRef.forward = &g_listenerFwd;
        listenerRef.up = &g_listenerUp;
        g_audioListener.BindTransform(listenerRef);

        AudioTransformRef sourceRef{};
        sourceRef.position = &g_sourcePos;
        sourceRef.velocity = &g_sourceVel;
        g_audioSource.BindTransform(sourceRef);

        g_audioSource.SetClip(g_audioClip);
        g_audioSource.SetLoop(entry->loop);
        g_audioSource.SetVolume(entry->defaultVolume);
        g_audioSource.Set3DMinMaxDistance(1.0f, 200.0f);

        g_audioReady = true;
    }
}

EngineApp::EngineApp(HINSTANCE hInstance)
	: GameApp(hInstance)
{
}

EngineApp::~EngineApp()
{
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
	ShaderManager::Instance().CreateCB(dxRenderer->GetDevice());
    AudioManager::Instance().Initialize();
    TrySetupAudioTest();

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
	CameraSystem::Instance().SetScreenSize(clientWidth, clientHeight);

#if _DEBUG
	auto freeCamHandle = CameraSystem::Instance().CreateFreeCamera(clientWidth, clientHeight, SceneSystem::Instance().GetCurrentScene().get());
    auto freeCamObjPtr = ObjectSystem::Instance().Get<GameObject>(freeCamHandle);
    freeCamObjPtr->AddComponent<FreeCamera>();
#endif

    // == find scene ==
    LoadSavedFirstScene();

	WorldManager::Instance().CreateDirectionalLightFrustum(); // create directional light 


	// == init renderpass ==
	// NOTE : 랜더링하는 순서대로 추가 할 것
	auto shadowPass = std::make_shared<ShadowRenderPass>();
	shadowPass->Init(dxRenderer->GetDevice(), dxRenderer->GetDeviceContext(), CameraSystem::Instance().GetCurrCamera());
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

#if _DEBUG
#else
    PlayModeSystem::Instance().SetPlayMode(PlayModeState::Playing);
#endif

	return true;
}

void EngineApp::OnUpdate()
{
	SceneSystem::Instance().BeforUpdate();	
	CameraSystem::Instance().FreeCameraUpdate(GameTimer::Instance().DeltaTime());
	WorldManager::Instance().Update();
	SceneSystem::Instance().UpdateScene(GameTimer::Instance().DeltaTime());
    AudioManager::Instance().Update();

    if (g_audioReady)
    {
        auto currCam = CameraSystem::Instance().GetCurrCamera();
        if (currCam && currCam->GetOwner())
        {
            auto transform = currCam->GetOwner()->GetTransform();
            if (transform)
            {
                const auto& pos = transform->GetPosition();
                g_listenerPos = { pos.x, pos.y, pos.z };
            }

            auto forward = currCam->GetForward();
            g_listenerFwd = { forward.x, forward.y, forward.z };
        }

        g_sourcePos = {
            g_listenerPos.x + g_listenerFwd.x * 5.0f,
            g_listenerPos.y + g_listenerFwd.y * 5.0f,
            g_listenerPos.z + g_listenerFwd.z * 5.0f
        };

        g_audioListener.Update();
        g_audioSource.Update3D();

        if (!g_audioStarted)
        {
            g_audioSource.Play();
            g_audioStarted = true;
        }
    }

#if _DEBUG
	editor->Update();
#endif
}

void EngineApp::OnRender()
{
	BeginRender(); 					// 업데이트 준비
	
	auto freeCam = CameraSystem::Instance().GetFreeCamera();	
    auto currCam = CameraSystem::Instance().GetCurrCamera();

	for(auto& pass : renderPasses)
	{	
        if (PlayModeSystem::Instance().IsPlaying())
        {
            if (typeid(*pass) == typeid(GBufferRenderPass))
            {
                dxRenderer->ProcessScene(SceneSystem::Instance().GetCurrentScene(), pass, currCam);
            }
            else if (typeid(*pass) == typeid(ShadowRenderPass))
            {
                dxRenderer->ProcessScene(SceneSystem::Instance().GetCurrentScene(), pass, currCam);
            }
            else
            {
                dxRenderer->ProcessScene(nullptr, pass, currCam);  // 렌더러가 씬을 렌더링
            }
        }
        else
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
}

// ================= 컴포넌트 등록 =================

#include "Components/FBXData.h"
#include "Components/FBXRenderer.h"
#include "Entity/Transform.h"
#include "Entity/Camera.h"
#include "System/ComponentFactory.h"

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
}
