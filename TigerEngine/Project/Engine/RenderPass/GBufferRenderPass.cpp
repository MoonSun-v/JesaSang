#include "GBufferRenderPass.h"
#include "Entity/GameObject.h"
#include "../Util/PathHelper.h"

namespace fs = std::filesystem;

struct ConstantBuffer   // TODO 정리하기
{
	Matrix cameraView;
	Matrix cameraProjection;

	Matrix shadowView;
	Matrix shadowProjection;

	Vector4 ambient;	// 환경광
	Vector4 diffuse;	// 난반사
	Vector4 specular;	// 정반사
	FLOAT shininess;	// 광택지수
	Vector3 CameraPos;	// 카메라 위치
};

void GBufferRenderPass::Init(const ComPtr<ID3D11Device> &device, const ComPtr<ID3D11DeviceContext>& deviceContext,  UINT width, UINT height)
{
    std::wstring assetPath = PathHelper::FindDirectory("Engine\\Shaders").value().wstring();
    bufferCount = static_cast<int>(EGbuffer::Count);
    clientWidth = width;
    clientHeight = height;

    CreateGBuffers(device); // RTV, SRV, Textures

    /* ----------------------------- inputlayout 만들기 ---------------------------- */
    D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

    /* -------------------------------- 셰이더 파일 생성 ------------------------------- */
    std::wstring path = assetPath + L"\\VS_GBuffer.hlsl";
    ComPtr<ID3DBlob> vertexShaderBuffer = nullptr;
    HR_T(CompileShaderFromFile(path.c_str(), "main", "vs_5_0", vertexShaderBuffer.GetAddressOf()));
    HR_T(device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), inputLayout.GetAddressOf()));
	HR_T(device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, vertexShader.GetAddressOf()));

    path = assetPath + L"\\PS_Gbuffer.hlsl";
    ComPtr<ID3DBlob> pixelShaderBuffer{};
	HR_T(CompileShaderFromFile(path.c_str(), "main", "ps_5_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, pixelShader.GetAddressOf()));

    /* ------------------------------ 래스터라이저 상태 만들기 ----------------------------- */
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.CullMode = D3D11_CULL_FRONT;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.DepthClipEnable = true;
	rasterizerDesc.FrontCounterClockwise = true;

	device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);

    /* --------------------------------- 샘플러 만들기 -------------------------------- */
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;	// 텍스처 샘플링할 때 사용할 필터링 방법
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;		// 범위 밖에 있는 텍스처 좌표 확인 방법
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;	// 샘플링된 데이터를 기존 데이터와 확인하는 방법
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    HR_T(device->CreateSamplerState(&sampDesc, samplerLinear.GetAddressOf()));

    sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.MipLODBias = 0.0f;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampDesc.BorderColor[0] = sampDesc.BorderColor[1] = sampDesc.BorderColor[2] = sampDesc.BorderColor[3] = 0.0f;
	sampDesc.MinLOD = 0.0f;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR_T(device->CreateSamplerState(&sampDesc, samplerPoint.GetAddressOf()));

    /* ------------------------------ 뎊스 스탠실 상태 만들기 ----------------------------- */
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = TRUE;                // 깊이 테스트 활성화
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; // 깊이 버퍼 업데이트 허용
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; // 작은 Z 값이 앞에 배치되도록 설정
	depthStencilDesc.StencilEnable = FALSE;            // 스텐실 테스트 비활성화

	HR_T(device->CreateDepthStencilState(&depthStencilDesc, &dpethStencliState));

    /* ------------------------------- 뷰포트 상태 만들기 ------------------------------- */
    renderViewport = {};
	renderViewport.TopLeftX = 0;
	renderViewport.TopLeftY = 0;
	renderViewport.Width = (float)clientWidth;
	renderViewport.Height = (float)clientHeight;
	renderViewport.MinDepth = 0.0f;
	renderViewport.MaxDepth = 1.0f;
	deviceContext->RSSetViewports(1, &renderViewport);

    /* -------------------------------- 상수 버퍼 만들기 ------------------------------- */
    D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(device->CreateBuffer(&bufferDesc, nullptr, cbCamera.GetAddressOf()));
}

void GBufferRenderPass::Execute(ComPtr<ID3D11DeviceContext> &context, std::shared_ptr<Scene> scene, Camera* cam)
{
    // GBuffer 초기화
	float clearValue[4] = { 0,0,0,0 };
	for (int i = 0; i < bufferCount; i++)
	{
		context->ClearRenderTargetView(gbufferRTVs[i].Get(), clearValue);
	}
	context->OMSetRenderTargets(bufferCount, gbufferRTVs.data()->GetAddressOf(), depthStencilView.Get());

	// Update Constant Values
	ConstantBuffer cb;
	cb.cameraView = XMMatrixTranspose(cam->GetView());
	cb.cameraProjection = XMMatrixTranspose(cam->GetProjection());

	cb.CameraPos = cam->GetOwner()->GetTransform()->GetPosition();
	context->UpdateSubresource(cbCamera.Get(), 0, nullptr, &cb, 0, 0);

	context->IASetInputLayout(inputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->VSSetShader(vertexShader.Get(), 0, 0);
	context->VSSetConstantBuffers(0, 1, cbCamera.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, cbCamera.GetAddressOf());

	context->RSSetState(rasterizerState.Get());
	context->RSSetViewports(1, &renderViewport);

	context->PSSetShader(pixelShader.Get(), 0, 0);
	context->PSSetSamplers(0, 1, samplerLinear.GetAddressOf());
	context->PSSetSamplers(1, 1, samplerPoint.GetAddressOf());
    context->OMSetDepthStencilState(dpethStencliState.Get(), 1);
}

void GBufferRenderPass::End(ComPtr<ID3D11DeviceContext> &context)
{
    // RTV Unbind 	
	vector<ID3D11RenderTargetView*> nullRTVs(gbufferRTVs.size(), {});
	context->OMSetRenderTargets(bufferCount, nullRTVs.data(), nullptr);
}

void GBufferRenderPass::SetDepthStencilView(const ComPtr<ID3D11DepthStencilView> &dsv)
{
    depthStencilView = dsv;
}

std::vector<ComPtr<ID3D11ShaderResourceView>>& GBufferRenderPass::GetShaderResourceViews()
{
    return gbufferSRVs;
}

void GBufferRenderPass::CreateGBuffers(const ComPtr<ID3D11Device> &device)
{
	gbufferRTVs.assign(bufferCount, {});
	gbufferSRVs.assign(bufferCount, {});
	gbufferTextures.assign(bufferCount, {});
	struct RTDesc
	{
		DXGI_FORMAT format;
	};

	vector<RTDesc> formats
	{
		{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB },	// BaseColor
		{ DXGI_FORMAT_R8G8B8A8_UNORM },			// Normal
		{ DXGI_FORMAT_R16G16B16A16_FLOAT },		// PositionWS 
		{ DXGI_FORMAT_R8_UNORM },			// Metal
		{ DXGI_FORMAT_R8_UNORM },			// rough
		{ DXGI_FORMAT_R8_UNORM },			// specular
		{ DXGI_FORMAT_R8G8B8A8_UNORM },			// emission
	};


	for (int i = 0; i < bufferCount; i++)
	{
		D3D11_TEXTURE2D_DESC ds{};
		ds.Width = clientWidth;
		ds.Height = clientHeight;
		ds.MipLevels = 1;
		ds.ArraySize = 1;
		ds.Format = formats[i].format;
		ds.SampleDesc.Count = 1;
		ds.Usage = D3D11_USAGE_DEFAULT;
		ds.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		HR_T(device->CreateTexture2D(&ds, nullptr, gbufferTextures[i].GetAddressOf()));
		HR_T(device->CreateShaderResourceView(gbufferTextures[i].Get(), nullptr, gbufferSRVs[i].GetAddressOf()));
		HR_T(device->CreateRenderTargetView(gbufferTextures[i].Get(), nullptr, gbufferRTVs[i].GetAddressOf()));
	}
}
