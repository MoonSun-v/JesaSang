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
#include <cwctype>

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

static bool EndsWithI(const std::wstring& s, const std::wstring& suffix)
{
    if (s.size() < suffix.size()) return false;
    const size_t off = s.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i)
    {
        wchar_t a = std::towlower(s[off + i]);
        wchar_t b = std::towlower(suffix[i]);
        if (a != b) return false;
    }
    return true;
}

HRESULT CreateTextureFromFile(ID3D11Device* d3dDevice, const wchar_t* szFileName, ID3D11ShaderResourceView** textureView, TextureColorSpace colorSpace)
{
    HRESULT hr = S_OK;
    *textureView = nullptr;

    // 파일 확장자 확인 (TGA만 TGA 로더로)
    std::wstring ext;
    try
    {
        ext = std::filesystem::path(szFileName).extension().wstring();
    }
    catch (...)
    {
        ext.clear(); // path 파싱 실패해도 아래 시도는 진행
    }

    // 1) DDS 먼저 시도 (확장자가 .dds일 때만 먼저 타도 됨. 그래도 지금처럼 "먼저 DDS" 시도해도 무방)
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
        dds_flag,
        nullptr,
        textureView
    );

    if (SUCCEEDED(hr) && *textureView)
        return S_OK;

    // 2) TGA는 WIC로 안 되는 경우가 많아서(.tga는 보통 WIC 미지원) 확장자가 tga면 바로 TGA 경로로
    if (EndsWithI(ext, L".tga"))
    {
        DirectX::ScratchImage img;
        hr = DirectX::LoadFromTGAFile(szFileName, nullptr, img);
        if (FAILED(hr))
            return hr;

        ID3D11Resource* tex = nullptr;
        hr = DirectX::CreateTexture(d3dDevice, img.GetImages(), img.GetImageCount(), img.GetMetadata(), &tex);
        if (FAILED(hr))
        {
            if (tex) tex->Release();
            return hr;
        }

        hr = d3dDevice->CreateShaderResourceView(tex, nullptr, textureView);
        tex->Release();

        return hr;
    }

    // 3) 그 외는 WIC (PNG, JPG, BMP, TIFF, etc)
    // WIC 플래그는 SRGB/LINEAR만 주고, 실패하면 FLAGS_NONE으로도 한 번 더 시도(환경 따라 메타데이터 처리에서 실패할 때가 있음)
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
        wic_flag,
        nullptr,
        textureView
    );

    if (SUCCEEDED(hr) && *textureView)
        return S_OK;

    // fallback: WIC FLAGS_NONE로 재시도
    hr = DirectX::CreateWICTextureFromFileEx(
        d3dDevice,
        szFileName,
        0,
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE,
        0,
        0,
        wic_flag,
        nullptr,
        textureView
    );

    if (SUCCEEDED(hr) && *textureView)
        return S_OK;

    return hr;
}