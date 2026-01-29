#include "GameApp.h"
#include "pch.h"
#include "Helper.h"
#include "System/InputSystem.h"
#include "System/TimeSystem.h"
#include "RendererPlatform/DirectX11Renderer.h"

// Debug 모듈관련 헤더 파일 및 라이브러리
// #include "Helper.h"
// #include <dbghelp.h>
// #include <minidumpapiset.h>
//  
// #pragma comment(lib, "Dbghelp.lib")

GameApp* GameApp::InstancePtr = nullptr;
HWND GameApp::hwnd;

#define USE_FLIPMODE 1

LRESULT CALLBACK DefaultWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return  GameApp::InstancePtr->WndProc(hWnd, message, wParam, lParam);
}

void CreateDump(EXCEPTION_POINTERS* pExceptionPointers)
{
    // 와이드 문자(wchar_t)와 와이드 문자열(std::wstring)을 사용하고 있으므로,
    // 명시적으로 유니코드 버전의 Win32 API 함수를 호출해야 합니다.

    wchar_t moduleFileName[MAX_PATH] = { 0, };
    std::wstring fileName; // 초기화 방법 변경 (아래 참고)

    // 1. GetModuleFileName 대신 GetModuleFileNameW 사용
    if (GetModuleFileNameW(NULL, moduleFileName, MAX_PATH) == 0) { 
        fileName = L"unknown_project.dmp"; // L 접두사 사용 유지
    }
    else
    {
        // wchar_t* 에서 std::wstring 생성
        fileName = std::wstring(moduleFileName); 
        size_t pos = fileName.find_last_of(L"\\/");
        // ... (나머지 문자열 처리 로직은 동일)
        if (pos != std::wstring::npos) {
            fileName = fileName.substr(pos + 1); 
        }

        pos = fileName.find_last_of(L'.');
        if (pos != std::wstring::npos) {
            fileName = fileName.substr(0, pos); 
        }
        fileName += L".dmp";
    }

    // 2. CreateFile 대신 CreateFileW 사용
    HANDLE hFile = CreateFileW(fileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return;

    CloseHandle(hFile);
}

LONG WINAPI CustomExceptionHandler(EXCEPTION_POINTERS* pExceptionPointers)
{
	int msgResult = MessageBox(
		NULL,
		L"Should Create Dump ?",
		L"Exception",
		MB_YESNO | MB_ICONQUESTION
	);

	if (msgResult == IDYES) {
		CreateDump(pExceptionPointers);
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

GameApp::GameApp(HINSTANCE hInstance)
	:hInstance(hInstance), windowClassName(L"DefaultWindowCalss"), titleName(L"GameApp"), clientWidth(1024), clientHeight(768)
{
	GameApp::InstancePtr = this;
	wcex.hInstance = hInstance;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = DefaultWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = windowClassName;
}

GameApp::~GameApp()
{

}

bool GameApp::Initialize(UINT Width, UINT Height)
{
	SetUnhandledExceptionFilter(CustomExceptionHandler);

	clientWidth = Width;
	clientHeight = Height;

	// 등록
	RegisterClassExW(&wcex);

	// 원하는 크기가 조정되어 리턴
	RECT rcClient = { 0, 0, (LONG)Width, (LONG)Height };
	AdjustWindowRect(&rcClient, WS_OVERLAPPEDWINDOW, FALSE);

	//생성
	hwnd = CreateWindowW(windowClassName, titleName, WS_OVERLAPPEDWINDOW,
		100, 100,	// 시작 위치
		rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
		nullptr, nullptr, hInstance, nullptr);

	if (!hwnd)
	{
		return false;
	}

	// 윈도우 보이기
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	// 시스템 초기화
	InputSystem::Instance().Initialize(hwnd, this);

	renderer = std::make_shared<DirectX11Renderer>();
	renderer->Initialize(hwnd, clientWidth, clientHeight);

	// App 초기화
	if (!OnInitialize()) return false;

	// 타이머 초기화
	GameTimer::Instance().Reset();

    ConsoleInitialize();

	return true;
}

void GameApp::Uninialize()
{
    ConsoleUninitalize();
}

bool GameApp::OnUnitialize()
{
    return true;
}

bool GameApp::OnInitialize()
{
	return true;
}

bool GameApp::Run()
{
	// PeekMessage 메세지가 있으면 true,없으면 false
	while (TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			//윈도우 메시지 처리 
			TranslateMessage(&msg); // 키입력관련 메시지 변환  WM_KEYDOWN -> WM_CHAR
			DispatchMessage(&msg);
		}
		else
		{
			Update();
			Render();
            LateUpdate();
		}
	}

    Uninialize();
	return 0;
}

void GameApp::Update()
{
	GameTimer::Instance().Tick();
	InputSystem::Instance().Update(Singleton<GameTimer>::Instance().DeltaTime());

    OnPreUpdate();
    FixedUpdate(); // 물리 업데이트
	OnUpdate();
}

void GameApp::OnFixedUpdate()
{

}

void GameApp::Render()
{
	OnRender();
}

void GameApp::FixedUpdate()
{
    OnFixedUpdate();
}

void GameApp::LateUpdate()
{
    OnLateUpdate();
}

void GameApp::OnPreUpdate()
{
}

void GameApp::OnUpdate()
{

}

void GameApp::OnLateUpdate()
{

}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK GameApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_ACTIVATEAPP:
		DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
		DirectX::Mouse::ProcessMessage(message, wParam, lParam);
		break;
	case WM_INPUT:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEHOVER:
		DirectX::Mouse::ProcessMessage(message, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		DirectX::Keyboard::ProcessMessage(message, wParam, lParam);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
	return 0;
}

void GameApp::OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker, const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker)
{
}