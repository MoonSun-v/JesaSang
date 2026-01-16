#include "SkyboxRenderPass.h"
#include "directxtk/DDSTextureLoader.h"
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

struct CubeVertex
{
	Vector3 position;
};

void SkyboxRenderPass::Init(const ComPtr<ID3D11Device>& device, const ComPtr<ID3D11DeviceContext>& deviceContext, UINT width, UINT height)
{
    clientWidth = width;
    clientHeight = height;
    CreateCube(device);

    /* ------------------------------ 래스터라이저 상태 만들기 ----------------------------- */
	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.CullMode = D3D11_CULL_BACK;
	rsDesc.FillMode = D3D11_FILL_SOLID;
	rsDesc.DepthClipEnable = true;
	rsDesc.FrontCounterClockwise = true;

	HR_T(device->CreateRasterizerState(&rsDesc, rasterizerState.GetAddressOf()));

    /* ------------------------------- 뎊스 스텐실 상태 만들기 ------------------------------- */
	D3D11_DEPTH_STENCIL_DESC skyboxDsDesc{};
	skyboxDsDesc.DepthEnable = true;
	skyboxDsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;	// 깊이 버퍼 사용 X
	skyboxDsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;		
	skyboxDsDesc.StencilEnable = false;

	HR_T(device->CreateDepthStencilState(&skyboxDsDesc, depthStencliState.GetAddressOf()));

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
	HR_T(device->CreateBuffer(&bufferDesc, nullptr, cameraCB.GetAddressOf()));    

    /* --------------------------------- 텍스처 로드 --------------------------------- */
    HR_T(CreateDDSTextureFromFile(device.Get(), L"..\\Assets\\Resource\\skyboxEnvHDR.dds", nullptr, skyboxTexture.GetAddressOf()));
}

void SkyboxRenderPass::Execute(ComPtr<ID3D11DeviceContext> &context, std::shared_ptr<Scene> scene, Camera* cam)
{
    // 카메라용 뷰 행렬과, 투영행렬
	Matrix m_skyboxProjection = XMMatrixPerspectiveFovLH(cam->GetPovAngle(), clientWidth / (FLOAT)clientHeight, 0.1, cam->GetFarDist());

	// Update Constant Values
	ConstantBuffer cb;
	cb.cameraView = XMMatrixTranspose(cam->GetView()); // 쉐이더 코드 내부에서 이동 성분 제거함
	cb.cameraProjection = XMMatrixTranspose(m_skyboxProjection);

	context->IASetVertexBuffers(0, 1, skyboxVertexBuffer.GetAddressOf(), &skyboxVertexBufferStride, &skyboxVertexBufferOffset);
	context->IASetIndexBuffer(skyboxIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(inputLayout.Get());

	context->UpdateSubresource(cameraCB.Get(), 0, nullptr, &cb, 0, 0);

	context->VSSetShader(vertexShader.Get(), nullptr, 0);
	context->VSSetConstantBuffers(0, 1, cameraCB.GetAddressOf());

	context->RSSetState(rasterizerState.Get());
	context->RSSetViewports(1, &renderViewport); // 뷰포트 되돌리기
    
	context->PSSetShader(pixelShader.Get(), nullptr, 0);
	context->PSSetShaderResources(5, 1, skyboxTexture.GetAddressOf());
	context->PSSetSamplers(0, 1, samplerLinear.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, cameraCB.GetAddressOf());

	context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencliView.Get());
	context->OMSetDepthStencilState(depthStencliState.Get(), 1); // 뎊스 스텐실 설정

	context->DrawIndexed(SkyboxIndicesCount, 0, 0);
}

void SkyboxRenderPass::End(ComPtr<ID3D11DeviceContext> &context)
{
}

void SkyboxRenderPass::SetDepthStencilView(const ComPtr<ID3D11DepthStencilView> &dsv)
{
    depthStencliView = dsv;
}

void SkyboxRenderPass::SetRenderTargetView(const ComPtr<ID3D11RenderTargetView> &rtv)
{
    renderTargetView = rtv;
}

void SkyboxRenderPass::CreateCube(const ComPtr<ID3D11Device>& device)
{
    const float width = 1.0f;
	const float height = 1.0f;
	const float depth = 1.0f;
	CubeVertex skyboxVertices[] =
	{
		{ Vector3(-width, -height, -depth) },
		{ Vector3(-width, +height, -depth) },
		{ Vector3(+width, +height, -depth) },
		{ Vector3(+width, -height, -depth) },

		{ Vector3(-width, -height, +depth) },
		{ Vector3(+width, -height, +depth) },
		{ Vector3(+width, +height, +depth) },
		{ Vector3(-width, +height, +depth) },

		{ Vector3(-width, +height, -depth) },
		{ Vector3(-width, +height, +depth) },
		{ Vector3(+width, +height, +depth) },
		{ Vector3(+width, +height, -depth) },

		{ Vector3(-width, -height, -depth) },
		{ Vector3(+width, -height, -depth) },
		{ Vector3(+width, -height, +depth) },
		{ Vector3(-width, -height, +depth) },

		{ Vector3(-width, -height, +depth) },
		{ Vector3(-width, +height, +depth) },
		{ Vector3(-width, +height, -depth) },
		{ Vector3(-width, -height, -depth) },

		{ Vector3(+width, -height, -depth) },
		{ Vector3(+width, +height, -depth) },
		{ Vector3(+width, +height, +depth) },
		{ Vector3(+width, -height, +depth) }
	};

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.ByteWidth = sizeof(CubeVertex) * ARRAYSIZE(skyboxVertices);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = skyboxVertices;
	HR_T(device->CreateBuffer(&bufferDesc, &vbData, skyboxVertexBuffer.GetAddressOf()));

	skyboxVertexBufferStride = sizeof(CubeVertex);
	skyboxVertexBufferOffset = 0;

	/* --------------------------------- 인풋 레이아웃 -------------------------------- */
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

    std::wstring shadersPath = PathHelper::FindDirectory("Engine\\Shaders").value().wstring();
    std::wstring path = shadersPath + L"\\VS_Skybox.hlsl";

	ComPtr<ID3DBlob> vertexShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(path.c_str(), "main", "vs_5_0", &vertexShaderBuffer));
	HR_T(device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), inputLayout.GetAddressOf()));

    /* --------------------------------- 버텍스 버퍼 --------------------------------- */
	HR_T(device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, vertexShader.GetAddressOf()));

    /* --------------------------------- 인덱스 버퍼 --------------------------------- */
	WORD skyboxIndices[] =
	{
		0, 1, 2,
		0, 2, 3,
		4, 5, 6,
		4, 6, 7,
		8, 9, 10,
		8, 10, 11,
		12, 13, 14,
		12, 14, 15,
		16, 17, 18,
		16, 18, 19,
		20, 21, 22,
		20, 22, 23,
	};

	bufferDesc.ByteWidth = sizeof(WORD) * ARRAYSIZE(skyboxIndices);
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.CPUAccessFlags = 0;

	skyboxVertexBufferStride = sizeof(CubeVertex); 	// 버텍스 버퍼의 정보
	skyboxVertexBufferOffset = 0;

	SkyboxIndicesCount = ARRAYSIZE(skyboxIndices); // 인덱스 개수 저장

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = skyboxIndices;
	HR_T(device->CreateBuffer(&bufferDesc, &ibData, skyboxIndexBuffer.GetAddressOf()));

	/* ------------------------------- 픽셀 셰이더 만들기 ------------------------------- */

    path = shadersPath + L"\\PS_Skybox.hlsl";

	ComPtr<ID3DBlob> sbPixelShaderBuffer = nullptr;
	HR_T(CompileShaderFromFile(path.c_str(), "main", "ps_5_0", &sbPixelShaderBuffer));
	HR_T(device->CreatePixelShader(sbPixelShaderBuffer->GetBufferPointer(), sbPixelShaderBuffer->GetBufferSize(), NULL, pixelShader.GetAddressOf()));
}