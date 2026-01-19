#pragma once
#include "pch.h"
#include "../Scene/Scene.h"
#include "../Components/Camera.h"
#include "../RenderQueue/RenderQueue.h"

/// <summary>
/// 렌더 패스가 상속받는 클래스 ( depth only pass 등등 ) 
/// </summary>
class IRenderPass
{
public:
	virtual ~IRenderPass() = default;

	virtual void Execute(
		ComPtr<ID3D11DeviceContext>& context,
		RenderQueue& queue,
		Camera* cam
	) = 0;

	virtual void End(ComPtr<ID3D11DeviceContext>& context) = 0;
};