#include "pch.h"
#include <string>
#include "Helper.h"
#include <comdef.h>
#include <d3dcompiler.h>
#include <directXTK/DDSTextureLoader.h>
#include <directXTK/WICTextureLoader.h>
#include <dxgidebug.h>
#include <dxgi1_3.h>    // DXGIGetDebugInterface1
#include <DirectXTex.h>

#pragma comment(lib, "dxguid.lib")  // 꼭 필요!
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")


std::wstring GetComErrorString(HRESULT hr)
{
	_com_error err(hr);

#if defined(UNICODE) || defined(_UNICODE)
	return std::wstring(err.ErrorMessage());
#else
	const char* msg = err.ErrorMessage();
	if (!msg)
		return std::wstring();
	int size_needed = MultiByteToWideChar(CP_ACP, 0, msg, -1, nullptr, 0);
	std::wstring wmsg(size_needed, L'\0');
	MultiByteToWideChar(CP_ACP, 0, msg, -1, &wmsg[0], size_needed);
	if (!wmsg.empty() && wmsg.back() == L'\0')
		wmsg.pop_back();
	return wmsg;
#endif
}


HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			MessageBoxA(NULL, (char*)pErrorBlob->GetBufferPointer(), "CompileShaderFromFile", MB_OK);
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}

HRESULT CreateTextureFromFile(ID3D11Device* d3dDevice, const wchar_t* szFileName, ID3D11ShaderResourceView** textureView, TextureColorSpace colorSpace)
{
    HRESULT hr = S_OK;

    // 1) DDS 먼저 시도
    DirectX::DDS_LOADER_FLAGS dds_flag =
        (colorSpace == TextureColorSpace::SRGB) ? DirectX::DDS_LOADER_FORCE_SRGB : DirectX::DDS_LOADER_IGNORE_SRGB;

    hr = DirectX::CreateDDSTextureFromFileEx(
        d3dDevice,
        szFileName,
        0,
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE,
        0,
        0,
        dds_flag,               // DirectX::DDS_LOADER_FORCE_SRGB / DirectX::DDS_LOADER_IGNORE_SRGB
        nullptr,
        textureView
    );

    if (SUCCEEDED(hr))
        return S_OK;


    // 2) WIC로 시도 (PNG, JPG, BMP, etc)
    DirectX::WIC_LOADER_FLAGS wic_flag =
        (colorSpace == TextureColorSpace::SRGB) ? DirectX::WIC_LOADER_FORCE_SRGB : DirectX::WIC_LOADER_IGNORE_SRGB;

    hr = DirectX::CreateWICTextureFromFileEx(
        d3dDevice,
        szFileName,
        0,
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE,
        0,
        0,
        wic_flag,           // DirectX::WIC_LOADER_FORCE_SRGB / DirectX::WIC_LOADER_IGNORE_SRGB
        nullptr,
        textureView
    );
    if (SUCCEEDED(hr))
        return S_OK;


    // 3) TGA 시도
    DirectX::ScratchImage img;
    hr = DirectX::LoadFromTGAFile(szFileName, nullptr, img);
    if (FAILED(hr))
    {
        (HR_T(hr));
        return hr;
    }

    ID3D11Resource* tex = nullptr;
    hr = DirectX::CreateTexture(d3dDevice, img.GetImages(), img.GetImageCount(), img.GetMetadata(), &tex);
    if (FAILED(hr))
    {
        (HR_T(hr));
        return hr;
    }

    hr = d3dDevice->CreateShaderResourceView(tex, nullptr, textureView);
    tex->Release();

    if (FAILED(hr))
    {
        (HR_T(hr));
        return hr;
    }

    return S_OK;
}
