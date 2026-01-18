#pragma once
#include "../pch.h"
#include "directxtk/Mouse.h"
#include "directxtk/Keyboard.h"
#include "Singleton.h"


using namespace DirectX;

class InputProcesser
{
public:
	virtual void OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker,
		const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker) = 0;
};

class InputSystem : public Singleton<InputSystem>
{
public:
	InputSystem(token) {};
	~InputSystem() = default;

	InputProcesser* m_pInputProcessers = nullptr;

	// input
    std::unique_ptr<DirectX::Keyboard>              m_Keyboard{};
	std::unique_ptr<DirectX::Mouse>                 m_Mouse{};
	DirectX::Keyboard::KeyboardStateTracker         m_KeyboardStateTracker{};
	DirectX::Mouse::ButtonStateTracker              m_MouseStateTracker{};

	DirectX::Mouse::State                           m_MouseState{};
	DirectX::Keyboard::State                        m_KeyboardState{};

	void Update(float DeltaTime);
    bool Initialize(HWND hWnd, InputProcesser* processer);

	// 사용 예: if (InputSystem::Instance().GetKeyDown(Keyboard::Keys::Space)) { ... }
	// 또는:    if (Input::GetKey(Keyboard::Keys::W)) { ... }

	bool GetKey(Keyboard::Keys key) const;
	bool GetKeyDown(Keyboard::Keys key) const;
	bool GetKeyUp(Keyboard::Keys key) const;

	// button: 0=왼쪽, 1=오른쪽, 2=휠, 3=X1, 4=X2
	bool GetMouseButton(int button) const;
	bool GetMouseButtonDown(int button) const;
	bool GetMouseButtonUp(int button) const;

	// 마우스 위치 (클라이언트 영역, 픽셀). positionMode가 RELATIVE일 때는 delta 의미.
	int GetMouseX() const { return m_MouseState.x; }
	int GetMouseY() const { return m_MouseState.y; }
	int GetMouseScroll() const { return m_MouseState.scrollWheelValue; }

    // MODE_ABSOLUTE : 절대 좌표 - 스크린 좌표 위치
    // MODE_RELATIVE : 상대 좌표 - 마우스의 이동량(delta)만 읽음. FPS 카메라 회전에 사용.
    void SetMouseMode(Mouse::Mode mode);
    Mouse::Mode GetMouseMode() const { return m_MouseState.positionMode; }
};

// 짧게 쓰기: OnUpdate() 안에서 Input::GetKeyDown(Keys::Space) 등
namespace Input
{
	inline bool GetKey(Keyboard::Keys k) { return InputSystem::Instance().GetKey(k); }
	inline bool GetKeyDown(Keyboard::Keys k) { return InputSystem::Instance().GetKeyDown(k); }
	inline bool GetKeyUp(Keyboard::Keys k) { return InputSystem::Instance().GetKeyUp(k); }
	inline bool GetMouseButton(int b) { return InputSystem::Instance().GetMouseButton(b); }
	inline bool GetMouseButtonDown(int b) { return InputSystem::Instance().GetMouseButtonDown(b); }
	inline bool GetMouseButtonUp(int b) { return InputSystem::Instance().GetMouseButtonUp(b); }
	inline int GetMouseX() { return InputSystem::Instance().GetMouseX(); }
	inline int GetMouseY() { return InputSystem::Instance().GetMouseY(); }
	inline int GetMouseScroll() { return InputSystem::Instance().GetMouseScroll(); }
    inline Mouse::Mode GetMouseMode() { return InputSystem::Instance().GetMouseMode(); }
}