#include "DirectX11Renderer.h"
#include <algorithm>
#include "Helper.h"
#include "../EngineSystem/RenderSystem.h"

// datas
#include "Datas/Mesh.h"
#include "Datas/Vertex.h"



#define USE_FLIPMODE 1

void DirectX11Renderer::Initialize(HWND hwnd, int width, int height)
{
    // swap chain setup struct
    DXGI_SWAP_CHAIN_DESC swapDesc = {};
    swapDesc.BufferCount = 1;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.OutputWindow = hwnd;
    swapDesc.Windowed = true;
    swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    // UNORM : PS에서 감마 인코딩 필요, UNORM_SRGB : PS 자동 감마 적용
    swapDesc.BufferDesc.Width = width;
    swapDesc.BufferDesc.Height = height;
    swapDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapDesc.SampleDesc.Count = 1;
    swapDesc.SampleDesc.Quality = 0;

    // deviec create debug flag
    UINT creationFlags = 0;
#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // create device, device context, swap chain
    HR_T(D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        creationFlags,
        NULL,
        NULL,
        D3D11_SDK_VERSION,
        &swapDesc,
        &swapChain,
        &device,
        NULL,
        &deviceContext));

    // create RTV					
    HR_T(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backbufferTex.GetAddressOf()));			// backbuffer get
    HR_T(device->CreateRenderTargetView(backbufferTex.Get(), NULL, backBufferRTV.GetAddressOf()));	    // RTV create														            // RTV에서 backbuffer texture 참조중 (메모리 관리)

    ID3D11RenderTargetView* rtv = backBufferRTV.Get();
    deviceContext->OMSetRenderTargets(1, &rtv, nullptr);	// render targetview  binding

    // create depth stencil view 
    {
        // texture
        D3D11_TEXTURE2D_DESC descDepth = {};
        descDepth.Width = width;
        descDepth.Height = height;
        descDepth.MipLevels = 1;
        descDepth.ArraySize = 1;
        descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS;
        descDepth.SampleDesc.Count = 1;
        descDepth.SampleDesc.Quality = 0;
        descDepth.Usage = D3D11_USAGE_DEFAULT;
        descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        descDepth.CPUAccessFlags = 0;
        descDepth.MiscFlags = 0;

        HR_T(device->CreateTexture2D(&descDepth, nullptr, depthStencilTexture.GetAddressOf()));

        // write DSV
        D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
        descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // D24 : Depth, S8 : Stencil
        descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        descDSV.Texture2D.MipSlice = 0;
        descDSV.Flags = 0;
        HR_T(device->CreateDepthStencilView(depthStencilTexture.Get(), &descDSV, depthStencilView.GetAddressOf()));

        // read only DSV
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvRODesc = descDSV;
        dsvRODesc.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;
        HR_T(device->CreateDepthStencilView(depthStencilTexture.Get(), &dsvRODesc, depthStencilReadOnlyView.GetAddressOf()));
    }

    // depth stencil SRV
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC descSRV = {};
        descSRV.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        descSRV.Texture2D.MostDetailedMip = 0;
        descSRV.Texture2D.MipLevels = 1;

        HR_T(device->CreateShaderResourceView(depthStencilTexture.Get(), &descSRV, depthSRV.GetAddressOf()));
    }


    // viewport
    {
        renderViewport = {};
        renderViewport.TopLeftX = 0;
        renderViewport.TopLeftY = 0;
        renderViewport.Width = (float)width;
        renderViewport.Height = (float)height;
        renderViewport.MinDepth = 0.0f;
        renderViewport.MaxDepth = 1.0f;
        deviceContext->RSSetViewports(1, &renderViewport);	// viewport binding
    }

}

void DirectX11Renderer::OnResize(int width, int height)
{
    // clear
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    backBufferRTV.Reset();
    depthStencilView.Reset();
    depthStencilReadOnlyView.Reset();
    backbufferTex.Reset();
    depthSRV.Reset();
    depthStencilTexture.Reset();

    deviceContext->ClearState();
    deviceContext->Flush();

    const UINT backBufferWidth = static_cast<UINT>(width);
    const UINT backBufferHeight = static_cast<UINT>(height);
    const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    const DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// SwapChain
    if (swapChain) // check swapchain exist
    {
        // 스왑체인 리사이즈
        HR_T(swapChain->ResizeBuffers(0, backBufferWidth, backBufferHeight, DXGI_FORMAT_UNKNOWN, 0));
    }
    else
    {
        return; // -> 해당 함수 사용시 스왑체인이 있다고 가정한다.
    }

    // create RTV					
    HR_T(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backbufferTex.GetAddressOf()));			// backbuffer get
    HR_T(device->CreateRenderTargetView(backbufferTex.Get(), NULL, backBufferRTV.GetAddressOf()));	    // RTV create														            // RTV에서 backbuffer texture 참조중 (메모리 관리)

    // create depth stencil view 
    {
        // texture
        D3D11_TEXTURE2D_DESC descDepth = {};
        descDepth.Width = backBufferWidth;
        descDepth.Height = backBufferHeight;
        descDepth.MipLevels = 1;
        descDepth.ArraySize = 1;
        descDepth.Format = DXGI_FORMAT_R24G8_TYPELESS;
        descDepth.SampleDesc.Count = 1;
        descDepth.SampleDesc.Quality = 0;
        descDepth.Usage = D3D11_USAGE_DEFAULT;
        descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        descDepth.CPUAccessFlags = 0;
        descDepth.MiscFlags = 0;

        HR_T(device->CreateTexture2D(&descDepth, nullptr, depthStencilTexture.GetAddressOf()));

        // write DSV
        D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
        descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // D24 : Depth, S8 : Stencil
        descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        descDSV.Texture2D.MipSlice = 0;
        descDSV.Flags = 0;
        HR_T(device->CreateDepthStencilView(depthStencilTexture.Get(), &descDSV, depthStencilView.GetAddressOf()));

        // read only DSV
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvRODesc = descDSV;
        dsvRODesc.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;
        HR_T(device->CreateDepthStencilView(depthStencilTexture.Get(), &dsvRODesc, depthStencilReadOnlyView.GetAddressOf()));
    }

    // viewport 재설정
    {
        renderViewport = {};
        renderViewport.TopLeftX = 0;
        renderViewport.TopLeftY = 0;
        renderViewport.Width = (float)backBufferWidth;
        renderViewport.Height = (float)backBufferHeight;
        renderViewport.MinDepth = 0.0f;
        renderViewport.MaxDepth = 1.0f;
        deviceContext->RSSetViewports(1, &renderViewport);
    }
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

ComPtr<ID3D11DepthStencilView> DirectX11Renderer::GetDepthStencilReadOnlyView() const
{
    return depthStencilReadOnlyView;
}

ComPtr<ID3D11ShaderResourceView> DirectX11Renderer::GetDepthSRV() const
{
    return depthSRV;
}

D3D11_VIEWPORT DirectX11Renderer::GetRenderViewPort() const
{
    return renderViewport;
}
