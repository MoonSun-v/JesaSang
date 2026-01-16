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

	void Register(InputProcesser* input);
    void UnRegister(InputProcesser* comp);

	void UpdateRegisterInput(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker,
		const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker);
private:
	std::vector<InputProcesser*> registered;
};