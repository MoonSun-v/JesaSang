#include "DirectionalLightPass.h"
#include "directxtk/DDSTextureLoader.h"
#include "../Object/GameObject.h"

#include "../EngineSystem/CameraSystem.h"
#include "../Manager/WorldManager.h"
#include "../Util/PathHelper.h"

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

struct LightDirectionCB
{
	Vector4 lightDirection;
	Color lightColor;
};

struct QuadVertex
{
	Vector3 position;
	Vector2 uv;

	QuadVertex(float x, float y, float z, float u, float v) : position(x, y, z), uv(u, v) {}
	QuadVertex(Vector3 p, Vector2 u) : position(p), uv(u) {}
};

void DirectionalLightPass::Init(const ComPtr<ID3D11Device> &device)
{
    std::wstring assetPath = PathHelper::FindDirectory("Engine\\Shaders").value().wstring();
    CreateQuad(device);

    /* ---------------------------- 인풋 레이아웃 만들기 ---------------------------- */
    D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

    /* --------------------------------- 셰이더 만들기 -------------------------------- */
	ComPtr<ID3DBlob> vertexShaderBuffer{};
	// Light Pass
    std::wstring path = assetPath + L"\\VS_DirectionalLight.hlsl";
	HR_T(CompileShaderFromFile(path.c_str(), "main", "vs_5_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(device->CreateInputLayout(layout, ARRAYSIZE(layout),vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), inputLayout.GetAddressOf()));
	HR_T(device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, vertexShader.GetAddressOf()));

    path = assetPath + L"\\PS_DirectionalLight.hlsl";
    ComPtr<ID3DBlob> pixelShaderBuffer{};
	HR_T(CompileShaderFromFile(path.c_str(), "main", "ps_5_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, pixelShader.GetAddressOf()));

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

    /* --------------------------------- DSV 만들기 -------------------------------- */
    // 뎊스 스탠실 상태 설정
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = FALSE;                // 깊이 테스트 활성화
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 깊이 버퍼 업데이트 허용
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS; // 작은 Z 값이 앞에 배치되도록 설정
	depthStencilDesc.StencilEnable = FALSE;            // 스텐실 테스트 비활성화

	HR_T(device->CreateDepthStencilState(&depthStencilDesc, dpethStencliState.GetAddressOf()));

    /* ------------------------------- 블랜드 상태 만들기 ------------------------------- */
    D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable			= TRUE;						
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HR_T(device->CreateBlendState(&blendDesc, blendState.GetAddressOf()));

    /* ------------------------------- IBL 텍스처 로드 ------------------------------- */

    std::wstring resourcePath = PathHelper::FindDirectory("Assets\\Resource").value().wstring();
    path = resourcePath + L"\\skyboxDiffuseHDR.dds";
    HR_T(CreateDDSTextureFromFile(device.Get(), path.c_str(), nullptr, IBLIrradiance.GetAddressOf()));
	HR_T(CreateDDSTextureFromFile(device.Get(), L"..\\Assets\\Resource\\skyboxSpecularHDR.dds", nullptr, IBLSpecular.GetAddressOf()));
	HR_T(CreateDDSTextureFromFile(device.Get(), L"..\\Assets\\Resource\\skyboxBrdf.dds", nullptr, IBLLookUpTable.GetAddressOf()));

    /* -------------------------------- 상수 버퍼 만들기 ------------------------------- */
    D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(LightDirectionCB);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(device->CreateBuffer(&bufferDesc, nullptr, lightDirectionBufferCB.GetAddressOf()));

    bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBuffer);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	HR_T(device->CreateBuffer(&bufferDesc, nullptr, cameraCB.GetAddressOf()));
}

void DirectionalLightPass::Execute(ComPtr<ID3D11DeviceContext>& context,
                                    RenderQueue& queue,
                                    Camera* cam)
{
	LightDirectionCB lightdirCB;
	lightdirCB.lightDirection = Vector4(lightDir.x, lightDir.y, lightDir.z, lightIntensity);
	lightdirCB.lightColor = lightColor;

	ConstantBuffer cb;
	cb.CameraPos = cam->GetOwner()->GetTransform()->GetPosition();
	cb.shadowView = XMMatrixTranspose(WorldManager::Instance().directionalLightView);
	cb.shadowProjection = XMMatrixTranspose(WorldManager::Instance().directionalLightProj);

	// 11, 12, 13 -> color, normal, worldpos
	// 4 shadow depth

	// 처음 빛을 추가할 때 백버퍼 초기화
	float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	context->OMSetBlendState(blendState.Get(), blendFactor, 0xffffffff);
	context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencliView.Get()); // backbuffer -> 스왑체인꺼 가져와야함
	context->OMSetDepthStencilState(dpethStencliState.Get(), 0); // Depth test OFF, write OFF

	std::vector<ID3D11ShaderResourceView*> SRVs =
	{
	    (*gbufferSRVs)[0].Get(),
	    (*gbufferSRVs)[1].Get(),
	    (*gbufferSRVs)[2].Get(),
	    (*gbufferSRVs)[3].Get(),
	    (*gbufferSRVs)[4].Get()
	};

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 1, quadVertexBuffer.GetAddressOf(), &quadVertexBufferStride, &quadVertexBufferOffset);
	context->IASetIndexBuffer(quadIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetInputLayout(inputLayout.Get());

	context->VSSetShader(vertexShader.Get(), nullptr, 0);

	context->UpdateSubresource(lightDirectionBufferCB.Get(), 0, nullptr, &lightdirCB, 0, 0);
	context->PSSetConstantBuffers(5, 1, lightDirectionBufferCB.GetAddressOf());
	context->UpdateSubresource(cameraCB.Get(), 0, nullptr, &cb, 0, 0);
	context->PSSetConstantBuffers(0, 1, cameraCB.GetAddressOf());

	context->PSSetShaderResources(11, SRVs.size(), SRVs.data());			// gbuffer texture 바인드
	context->PSSetShaderResources(4, 1, shadowSRV.GetAddressOf());	// shadow
	context->PSSetShaderResources(8, 1, IBLIrradiance.GetAddressOf());	// Irradiance
	context->PSSetShaderResources(9, 1, IBLSpecular.GetAddressOf());		// Sepcular
	context->PSSetShaderResources(10, 1, IBLLookUpTable.GetAddressOf());	// LUT

	context->PSSetSamplers(0, 1, samplerLinear.GetAddressOf());
	context->PSSetSamplers(1, 1, samplerPoint.GetAddressOf());
	context->PSSetShader(pixelShader.Get(), nullptr, 0); 
	context->DrawIndexed(quadIndicesCount, 0, 0);
}

void DirectionalLightPass::End(ComPtr<ID3D11DeviceContext> &context)
{
    float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    context->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	
	// srv unbind
	std::vector<ID3D11ShaderResourceView*> nullSRVs(gbufferSRVs->size(), nullptr);
	context->PSSetShaderResources(11, gbufferSRVs->size(), nullSRVs.data());	// gbuffer tex
	
	context->PSSetShaderResources(4, 1, &nullSRVs[0]);
}

void DirectionalLightPass::SetClient(UINT width, UINT height)
{
}

void DirectionalLightPass::SetGBufferSRV(std::vector<ComPtr<ID3D11ShaderResourceView>> &srvs)
{
    gbufferSRVs = &srvs;
}

void DirectionalLightPass::SetDepthStencilView(const ComPtr<ID3D11DepthStencilView> &dsv)
{
    depthStencliView = dsv;
}

void DirectionalLightPass::SetRenderTargetView(const ComPtr<ID3D11RenderTargetView> &rtv)
{
    renderTargetView = rtv;
}

void DirectionalLightPass::SetShadowSRV(const ComPtr<ID3D11ShaderResourceView> &srv)
{
	shadowSRV = srv;
}

void DirectionalLightPass::CreateQuad(const ComPtr<ID3D11Device> &device)
{
	QuadVertex QuadVertices[] =
	{
		QuadVertex(Vector3(-1.0f,  1.0f, 1.0f), Vector2(0.0f,0.0f)),	// Left Top 
		QuadVertex(Vector3(1.0f,  1.0f, 1.0f), Vector2(1.0f, 0.0f)),	// Right Top
		QuadVertex(Vector3(-1.0f, -1.0f, 1.0f), Vector2(0.0f, 1.0f)),	// Left Bottom
		QuadVertex(Vector3(1.0f, -1.0f, 1.0f), Vector2(1.0f, 1.0f))		// Right Bottom
	};

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.ByteWidth = sizeof(QuadVertex) * ARRAYSIZE(QuadVertices);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = QuadVertices;	// 배열 데이터 할당.
	HR_T(device->CreateBuffer(&vbDesc, &vbData, &quadVertexBuffer));
	quadVertexBufferStride = sizeof(QuadVertex);		// 버텍스 버퍼 정보
	quadVertexBufferOffset = 0;

	// InputLayout 생성 	
	D3D11_INPUT_ELEMENT_DESC layout[] = // 입력 레이아웃.
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	ComPtr<ID3DBlob> vertexShaderBuffer{};
	// Light Pass
	vertexShaderBuffer.Reset();
    std::wstring shadersPath = PathHelper::FindDirectory("Engine\\Shaders").value().wstring();
    std::wstring path = shadersPath + L"\\VS_DirectionalLight.hlsl";
	HR_T(CompileShaderFromFile(path.c_str(), "main", "vs_5_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(device->CreateInputLayout(layout, ARRAYSIZE(layout),vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), inputLayout.GetAddressOf()));
	HR_T(device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, vertexShader.GetAddressOf()));

	// 인덱스 버퍼 생성
	WORD indices[] =
	{
		0,1,2,
		1,3,2
	};
	quadIndicesCount = ARRAYSIZE(indices);	// 인덱스 개수 저장.
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.ByteWidth = sizeof(WORD) * ARRAYSIZE(indices);
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;
	HR_T(device->CreateBuffer(&ibDesc, &ibData, quadIndexBuffer.GetAddressOf()));
}