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

void InputSystem::Register(InputProcesser *input)
{
	registered.push_back(input);
}

void InputSystem::UnRegister(InputProcesser* comp)
{
    for (auto it = registered.begin(); it != registered.end(); it++)
    {
        if (*it == comp)
        {
            registered.erase(it);
            break;
        }
    }
}

void InputSystem::UpdateRegisterInput(const Keyboard::State &KeyState, const Keyboard::KeyboardStateTracker &KeyTracker, const Mouse::State &MouseState, const Mouse::ButtonStateTracker &MouseTracker)
{
	for(auto& e : registered)
	{
		e->OnInputProcess(KeyState, KeyTracker, MouseState, MouseTracker);
	}
}
