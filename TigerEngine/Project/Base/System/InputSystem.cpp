#include "../pch.h"
#include "InputSystem.h"

constexpr float ROTATION_GAIN = 0.004f;
constexpr float MOVEMENT_GAIN = 0.07f;

using namespace DirectX;
using namespace DirectX::SimpleMath;

void InputSystem::Update(float DeltaTime)
{
	m_MouseState = m_Mouse->GetState();
	m_MouseStateTracker.Update(m_MouseState);

	m_KeyboardState = m_Keyboard->GetState();
	m_KeyboardStateTracker.Update(m_KeyboardState);

	if (m_pInputProcessers != nullptr)
	{
		m_pInputProcessers->OnInputProcess(m_KeyboardState, m_KeyboardStateTracker, m_MouseState, m_MouseStateTracker);
	}
}

bool InputSystem::Initialize(HWND hWnd, InputProcesser* processer)
{
	m_Keyboard = std::make_unique<Keyboard>();
	m_Mouse = std::make_unique<Mouse>();
	m_Mouse->SetWindow(hWnd);
	m_pInputProcessers = processer;
	return true;
}

void InputSystem::SetMouseMode(Mouse::Mode mode)
{
	if (m_Mouse)
		m_Mouse->SetMode(mode);

    // m_MouseState.positionMode = mode;
}

bool InputSystem::GetKey(Keyboard::Keys key) const
{
	return m_KeyboardState.IsKeyDown(key);
}

bool InputSystem::GetKeyDown(Keyboard::Keys key) const
{
	return m_KeyboardStateTracker.IsKeyPressed(key);
}

bool InputSystem::GetKeyUp(Keyboard::Keys key) const
{
	return m_KeyboardStateTracker.IsKeyReleased(key);
}

bool InputSystem::GetMouseButton(int button) const
{
	switch (button)
	{
	case 0: return m_MouseState.leftButton;
	case 1: return m_MouseState.rightButton;
	case 2: return m_MouseState.middleButton;
	case 3: return m_MouseState.xButton1;
	case 4: return m_MouseState.xButton2;
	default: return false;
	}
}

bool InputSystem::GetMouseButtonDown(int button) const
{
	using B = Mouse::ButtonStateTracker;
	switch (button)
	{
	case 0: return m_MouseStateTracker.leftButton == B::PRESSED;
	case 1: return m_MouseStateTracker.rightButton == B::PRESSED;
	case 2: return m_MouseStateTracker.middleButton == B::PRESSED;
	case 3: return m_MouseStateTracker.xButton1 == B::PRESSED;
	case 4: return m_MouseStateTracker.xButton2 == B::PRESSED;
	default: return false;
	}
}

bool InputSystem::GetMouseButtonUp(int button) const
{
	using B = Mouse::ButtonStateTracker;
	switch (button)
	{
	case 0: return m_MouseStateTracker.leftButton == B::RELEASED;
	case 1: return m_MouseStateTracker.rightButton == B::RELEASED;
	case 2: return m_MouseStateTracker.middleButton == B::RELEASED;
	case 3: return m_MouseStateTracker.xButton1 == B::RELEASED;
	case 4: return m_MouseStateTracker.xButton2 == B::RELEASED;
	default: return false;
	}
}
