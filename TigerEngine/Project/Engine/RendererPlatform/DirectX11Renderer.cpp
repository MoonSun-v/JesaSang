#include "DirectX11Renderer.h"
#include <algorithm>
#include "Helper.h"
#include "../EngineSystem/RenderSystem.h"

// datas
#include "Datas/Mesh.h"
#include "Datas/Vertex.h"
#include "Datas/MaterialData.h"
#include "Datas/TransformData.h"

struct ConstantBuffer
{
	Matrix cameraView;
	Matrix cameraProjection;

	Vector4 lightDirection;
	Matrix shadowView;
	Matrix shadowProjection;

	Color lightColor;

	Vector4 ambient;	// 환경광
	Vector4 diffuse;	// 난반사
	Vector4 specular;	// 정반사
	FLOAT shininess;	// 광택지수
	Vector3 CameraPos;	// 카메라 위치
};


#define USE_FLIPMODE 1

void DirectX11Renderer::Initialize(HWND hwnd, int width, int height)
{
	UINT creationFlag = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
	creationFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif //  _DEBUG

	D3D_FEATURE_LEVEL featureLevels[] =
	{	
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0,D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_11_0
	};
	D3D_FEATURE_LEVEL actualFeatureLevel;

	HR_T(D3D11CreateDevice
	(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		0,
		creationFlag,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&device,
		&actualFeatureLevel,
		&deviceContext
	));

	UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG

	ComPtr<IDXGIFactory2> pFactory;
	HR_T(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&pFactory)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
#if USE_FLIPMODE == 1
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
#else
	swapChainDesc.BufferCount = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
#endif
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;

	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; 
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainDesc.Scaling = DXGI_SCALING_NONE; // 

	HR_T(pFactory->CreateSwapChainForHwnd
	(
		device.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		swapChain.GetAddressOf()
	));

	ComPtr<ID3D11Texture2D> pBackBufferTexture;
	HR_T(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBufferTexture));
	HR_T(device->CreateRenderTargetView(pBackBufferTexture.Get(), nullptr, backBufferRTV.GetAddressOf()));

	renderViewport = {};
	renderViewport.TopLeftX = 0;
	renderViewport.TopLeftY = 0;
	renderViewport.Width = (float)width;
	renderViewport.Height = (float)height;
	renderViewport.MinDepth = 0.0f;
	renderViewport.MaxDepth = 1.0f;
	deviceContext->RSSetViewports(1, &renderViewport);

	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // https://learn.microsoft.com/ko-kr/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;

	// create depthStencil texture
	ComPtr<ID3D11Texture2D> pTextureDepthStencil;
	HR_T(device->CreateTexture2D(&descDepth, nullptr, pTextureDepthStencil.GetAddressOf()));

	// create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	HR_T(device->CreateDepthStencilView(pTextureDepthStencil.Get(), &descDSV, depthStencilView.GetAddressOf()));
}

void DirectX11Renderer::OnResize(int width, int height)
{
	// SwapChain
	swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

	// RTV 
	//renderTargetView.Reset();
	ComPtr<ID3D11Texture2D> pBackBufferTexture;
	HR_T(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBufferTexture));
	HR_T(device->CreateRenderTargetView(pBackBufferTexture.Get(), nullptr, backBufferRTV.GetAddressOf()));
}

void DirectX11Renderer::BeginRender()
{
#if USE_FLIPMODE == 1
	deviceContext->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
#endif	
	Color color(0.0f, 0.0f, 0.0f, 0.0f);
	deviceContext->ClearRenderTargetView(backBufferRTV.Get(), color);
	deviceContext->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void DirectX11Renderer::EndRender()
{
	swapChain->Present(0, 0);
}

void DirectX11Renderer::ProcessScene
	(RenderQueue& queue, IRenderPass& renderPass, Camera* cam)
{
	renderPass.Execute(deviceContext, queue, cam);
	renderPass.End(deviceContext);
}

ComPtr<ID3D11Device> DirectX11Renderer::GetDevice() const
{
    return device;
}

ComPtr<ID3D11DeviceContext> DirectX11Renderer::GetDeviceContext() const
{
    return deviceContext;
}

ComPtr<ID3D11RenderTargetView> DirectX11Renderer::GetBackBufferRTV() const
{
    return backBufferRTV;
}

ComPtr<ID3D11DepthStencilView> DirectX11Renderer::GetDepthStencilView() const
{
    return depthStencilView;
}
