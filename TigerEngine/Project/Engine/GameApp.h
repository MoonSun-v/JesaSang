#pragma once
#include "pch.h"
#include <windows.h>
#include "System/TimeSystem.h"
#include "System/InputSystem.h"
#include "Renderer/IRenderer.h"

#define MAX_LOADSTRING 100

using Microsoft::WRL::ComPtr;

class GameApp : public InputProcesser
{
public:
	GameApp(HINSTANCE hInstance);
	virtual ~GameApp();

	static HWND		hwnd;
	static GameApp* InstancePtr;

public:
	HACCEL			hAccelTable{};
	MSG				msg{};
	HINSTANCE		hInstance{};
	WCHAR			titleName[MAX_LOADSTRING];         
	WCHAR			windowClassName[MAX_LOADSTRING];   
	WNDCLASSEXW		wcex{};								
	int				cmdShowCount{};
	UINT			clientWidth{};						
	UINT			clientHeight{};						

	std::shared_ptr<IRenderer>	renderer{};

	UINT internalWidth = 1920;
	UINT internalHeight = 1080;
	bool isResize = false;

protected:
    float m_PhysicsAccumulator = 0.0f;

public:
	bool Initialize(UINT Width, UINT Height);
    void Uninialize();
    virtual bool OnUnitialize();
	virtual bool OnInitialize();

	virtual bool Run();

	void Update();
	void Render();
    void FixedUpdate();
	virtual void OnUpdate();		// Update
    virtual void OnFixedUpdate();
	virtual void OnRender() = 0;	// Render

    void ConsoleInitialize();
    void ConsoleUninitalize();

	virtual LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker,
		const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker);
};