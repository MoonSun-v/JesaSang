#pragma once

// dxgi 어뎁터 조회용
#include <dxgi1_6.h>
#include <psapi.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "psapi.lib")

#include "pch.h"
#include "Renderer/IRenderer.h"
#include "../Scene/Scene.h"
#include "../RenderPass/IRenderPass.h"
#include "../Components/Camera.h"

/// <summary>
/// DirectX11 기능을 사용하는 클래스
/// </summary>
class DirectX11Renderer : public IRenderer
{
public:
	void Initialize(HWND hwnd, int width, int height) override;
	void OnResize(int width, int height) override;
	void BeginRender() override;
	void EndRender() override;

	// 매 랜더링 시 호출되는 함수
	void ProcessScene(RenderQueue& queue, 
					  IRenderPass& renderPass,
					  Camera* cam);

	ComPtr<ID3D11Device> GetDevice() const;
	ComPtr<ID3D11DeviceContext> GetDeviceContext() const;
	ComPtr<ID3D11RenderTargetView> GetBackBufferRTV() const;
	ComPtr<ID3D11DepthStencilView> GetDepthStencilView() const;
	
private:
	ComPtr<ID3D11Device>			device{};			// 디바이스
	ComPtr<ID3D11DeviceContext>		deviceContext{};	// 디바이스 컨텍스트
	ComPtr<IDXGISwapChain1>			swapChain{};		// 스왑체인 
	ComPtr<ID3D11RenderTargetView>	backBufferRTV{};	// 랜더 타겟	

	D3D11_VIEWPORT					renderViewport{};
	ComPtr<ID3D11DepthStencilView>	depthStencilView{};	// 뎊스 스텐실 뷰
};

