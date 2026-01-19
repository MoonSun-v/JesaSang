#include "ShaderManager.h"
#include <Datas/Vertex.h>
#include <Datas/FBXResourceData.h>
#include "../Base/Datas/ConstantBuffer.hpp"
#include "../Base/Datas/Vertex.h"

void ShaderManager::Init(const ComPtr<ID3D11Device>& dev, const ComPtr<ID3D11DeviceContext>& ctx
                                , float width, float height)
{
    CreateDSS(dev);
    CreateRS(dev);
    CreateSampler(dev);
    CreateBS(dev);
    CreateShadowResource(dev);
    CreateHDRResource(dev, width, height);
    CreateGbufferResource(dev, width, height);
    CreateBloomResource(dev, width, height);
    CreateInputLayoutShader(dev, ctx);
    CreateCB(dev);
}


// [ Create Funcs ] --------------------------------------------------------------
void ShaderManager::CreateDSS(const ComPtr<ID3D11Device>& dev)
{
    // create DSS (depth test on + write on)
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = TRUE;                              // 깊이 테스트 o  
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;     // 버퍼 기록 o
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
        dsDesc.StencilEnable = FALSE;

        HR_T(dev->CreateDepthStencilState(&dsDesc, defualtDSS.GetAddressOf()));
    }

    // create DSS (depth test only)
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = TRUE;                              // 깊이 테스트 o  
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;    // 버퍼 기록 x
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        dsDesc.StencilEnable = FALSE;

        HR_T(dev->CreateDepthStencilState(&dsDesc, depthTestOnlyDSS.GetAddressOf()));
    }

    // create DSS (depth test only / stencil write on (stencil test ALWAYS))
    // Light Volume Stencil Pass
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = TRUE;                              // Depth Test ON                          
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;    // 버퍼 기록 x        
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;         // 라이트 볼륨 표면이 씬 표면보다 앞이거나 같으면 통과    

        dsDesc.StencilEnable = TRUE;        // Stencil Test ON
        dsDesc.StencilReadMask = 0xFF;
        dsDesc.StencilWriteMask = 0xFF;     // Write ON

        // Front Face
        dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;       // Stencil Test 무조건 통과
        dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;       // 변경 x
        dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;  // 변경 x
        dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;    // Depth, Stencil 통과시 Ref로 Stencil 값 변경

        // Back Face (동일)
        dsDesc.BackFace = dsDesc.FrontFace;

        HR_T(dev->CreateDepthStencilState(&dsDesc, depthTestStencilWriteDSS.GetAddressOf()));
    }

    // create DSS (stencil test only)
    // Llight Volume Light Pass
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = FALSE;                                 // Depth Test OFF
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;

        dsDesc.StencilEnable = TRUE;       // Stencil Test ON
        dsDesc.StencilReadMask = 0xFF;
        dsDesc.StencilWriteMask = 0x00;    // Write OFF

        // Front Face
        dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;      // stencil == 1(Ref)인 픽셀만 통과
        dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;

        // Back Face (동일)
        dsDesc.BackFace = dsDesc.FrontFace;

        HR_T(dev->CreateDepthStencilState(
            &dsDesc,
            stencilTestOnlyDSS.GetAddressOf()
        ));
    }

    // create DSS (all disable)
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = FALSE;                              // 깊이 테스트 x
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;     // 버퍼 기록 x
        dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        dsDesc.StencilEnable = FALSE;

        HR_T(dev->CreateDepthStencilState(&dsDesc, disableDSS.GetAddressOf()));
    }
}

void ShaderManager::CreateRS(const ComPtr<ID3D11Device>& dev)
{
    // create RS (skybox 큐브의 안쪽이 그려지도록 cull mode front)
    {
        D3D11_RASTERIZER_DESC rsDesc = {};
        rsDesc.FillMode = D3D11_FILL_SOLID;
        rsDesc.CullMode = D3D11_CULL_FRONT;
        rsDesc.DepthClipEnable = TRUE;
        HR_T(dev->CreateRasterizerState(&rsDesc, cullfrontRS.GetAddressOf()));
    }

    // create RS
    {
        D3D11_RASTERIZER_DESC rsDesc = {};
        rsDesc.FillMode = D3D11_FILL_SOLID;
        rsDesc.CullMode = D3D11_CULL_NONE;
        rsDesc.FrontCounterClockwise = FALSE;
        rsDesc.DepthClipEnable = TRUE;
        HR_T(dev->CreateRasterizerState(&rsDesc, cullNoneRS.GetAddressOf()));
    }
}

void ShaderManager::CreateSampler(const ComPtr<ID3D11Device>& dev)
{
    // create smapler state (linear)
    {
        D3D11_SAMPLER_DESC sample_Desc = {};
        sample_Desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;			// 상하좌우 텍셀 보간
        sample_Desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;				// 0~1 범위를 벗어난 uv는 소수 부분만 사용
        sample_Desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sample_Desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sample_Desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sample_Desc.MinLOD = 0;
        sample_Desc.MaxLOD = D3D11_FLOAT32_MAX;
        HR_T(dev->CreateSamplerState(&sample_Desc, linearSamplerState.GetAddressOf()));
    }

    // create smapler state (clamp)
    {
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        HR_T(dev->CreateSamplerState(&sampDesc, shadowSamplerState.GetAddressOf()));
    }

    // create smapler state (linear + clamp) 
    {
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.MipLODBias = 0.0f;
        sampDesc.MaxAnisotropy = 1;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0.0f;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        HR_T(dev->CreateSamplerState(&sampDesc, linearClamSamplerState.GetAddressOf()));
    }

}

void ShaderManager::CreateBS(const ComPtr<ID3D11Device>& dev)
{
    // create blend state
    {
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        dev->CreateBlendState(&blendDesc, alphaBlendState.GetAddressOf());
    }

    // create blend state
    {
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        dev->CreateBlendState(&blendDesc, additiveBlendState.GetAddressOf());
    }
}

void ShaderManager::CreateShadowResource(const ComPtr<ID3D11Device>& dev)
{
    // create shadowDSV, shadowSRV
    // viewport
    viewport_shadowMap = {};
    viewport_shadowMap.TopLeftX = 0;
    viewport_shadowMap.TopLeftY = 0;
    viewport_shadowMap.Width = (float)8192;
    viewport_shadowMap.Height = (float)8192;
    viewport_shadowMap.MinDepth = 0.0f;
    viewport_shadowMap.MaxDepth = 1.0f;

    // texture2D
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = 8192;
    texDesc.Height = 8192;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R32_TYPELESS;        // DSV와 SRV가 TYPELESS 텍스처 공유
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL |    // 깊이값 기록 용도
        D3D11_BIND_SHADER_RESOURCE;   // 셰이더에서 텍스처 슬롯에 설정할 용도
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;

    HRESULT hr = dev->CreateTexture2D(&texDesc, nullptr, shadowMap.GetAddressOf());
    if (FAILED(hr)) { OutputDebugStringA("FAILED Create ShadowMapTexture"); }

    // DSV
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = dev->CreateDepthStencilView(shadowMap.Get(), &dsvDesc, shadowDSV.GetAddressOf());
    if (FAILED(hr)) { OutputDebugStringA("FAILED Create Shadow Depth Stencil View"); }

    // SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    hr = dev->CreateShaderResourceView(shadowMap.Get(), &srvDesc, shadowSRV.GetAddressOf());
    if (FAILED(hr)) { OutputDebugStringA("FAILED Create Shadow Shader Resource View"); }
}

void ShaderManager::CreateHDRResource(const ComPtr<ID3D11Device>& dev, int width, int height)
{
    // create HDR RTV, SRV
    // texture
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;       // HDR 포멧
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = dev->CreateTexture2D(&texDesc, nullptr, sceneHDRTex.GetAddressOf());
    if (FAILED(hr)) { OutputDebugStringA("FAILED Create HDR Texture"); }

    // RTV
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;       // HDR 포멧
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    hr = dev->CreateRenderTargetView(sceneHDRTex.Get(), &rtvDesc, sceneHDRRTV.GetAddressOf());
    if (FAILED(hr)) { OutputDebugStringA("FAILED Create HDR RTV"); }

    // SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;       // HDR 포멧
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    hr = dev->CreateShaderResourceView(sceneHDRTex.Get(), &srvDesc, sceneHDRSRV.GetAddressOf());
    if (FAILED(hr)) { OutputDebugStringA("FAILED Create HDR SRV"); }
}


// G-buffer util funcs
void CreateRTTex_RTV_SRV(const ComPtr<ID3D11Device>& device, int w, int h, DXGI_FORMAT fomat,
    ID3D11Texture2D** outTex, ID3D11RenderTargetView** outRTV, ID3D11ShaderResourceView** outSRV)
{
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w;
    td.Height = h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = fomat;
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;
    td.MiscFlags = 0;

    HRESULT hr = device->CreateTexture2D(&td, nullptr, outTex);
    hr = device->CreateRenderTargetView(*outTex, nullptr, outRTV);
    hr = device->CreateShaderResourceView(*outTex, nullptr, outSRV);
}

void ShaderManager::CreateGbufferResource(const ComPtr<ID3D11Device>& dev, int screenWidth, int screenHeight)
{
    const DXGI_FORMAT ALBEDO_FMT = DXGI_FORMAT_R8G8B8A8_UNORM;
    const DXGI_FORMAT NORMAL_FMT = DXGI_FORMAT_R16G16B16A16_FLOAT;
    const DXGI_FORMAT METALROUGH_FMT = DXGI_FORMAT_R8G8B8A8_UNORM;
    const DXGI_FORMAT EMISSIVE_FMT = DXGI_FORMAT_R16G16B16A16_FLOAT;

    // Albedo
    CreateRTTex_RTV_SRV(dev, screenWidth, screenHeight,
        ALBEDO_FMT,
        albedoTex.GetAddressOf(),
        albedoRTV.GetAddressOf(),
        albedoSRV.GetAddressOf());

    // Normal
    CreateRTTex_RTV_SRV(dev, screenWidth, screenHeight,
        NORMAL_FMT,
        normalTex.GetAddressOf(),
        normalRTV.GetAddressOf(),
        normalSRV.GetAddressOf());

    // Metal/Rough
    CreateRTTex_RTV_SRV(dev, screenWidth, screenHeight,
        METALROUGH_FMT,
        metalRoughTex.GetAddressOf(),
        metalRoughRTV.GetAddressOf(),
        metalRoughSRV.GetAddressOf());

    // Emissive
    CreateRTTex_RTV_SRV(dev, screenWidth, screenHeight,
        EMISSIVE_FMT,
        emissiveTex.GetAddressOf(),
        emissiveRTV.GetAddressOf(),
        emissiveSRV.GetAddressOf());
}

void ShaderManager::CreateBloomResource(const ComPtr<ID3D11Device>& dev, int screenWidth, int screenHeight)
{
    // create Bloom SRV, RTVs
   // half-res
    bloomW = std::max<UINT>(1, screenWidth / 2);
    bloomH = std::max<UINT>(1, screenHeight / 2);

    // Mip Count
    UINT w = bloomW;
    UINT h = bloomH;
    bloomMipCount = 1;
    while (w > 1 && h > 1)
    {
        w = std::max<UINT>(1, w >> 1);
        h = std::max<UINT>(1, h >> 1);
        ++bloomMipCount;
        if (bloomMipCount >= 6) break;
    }

    // Texture
    D3D11_TEXTURE2D_DESC td{};
    td.Width = bloomW;
    td.Height = bloomH;
    td.MipLevels = bloomMipCount;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R11G11B10_FLOAT;
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;
    td.MiscFlags = 0;

    HRESULT hr = S_OK;
    hr = dev->CreateTexture2D(&td, nullptr, bloomATex.GetAddressOf());
    hr = dev->CreateTexture2D(&td, nullptr, bloomBTex.GetAddressOf());
    hr = dev->CreateTexture2D(&td, nullptr, accumATex.GetAddressOf());
    hr = dev->CreateTexture2D(&td, nullptr, accumBTex.GetAddressOf());

    // SRV
    D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
    sd.Format = DXGI_FORMAT_R11G11B10_FLOAT;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MostDetailedMip = 0;
    sd.Texture2D.MipLevels = bloomMipCount;

    hr = dev->CreateShaderResourceView(bloomATex.Get(), &sd, bloomASRV.GetAddressOf());
    hr = dev->CreateShaderResourceView(bloomBTex.Get(), &sd, bloomBSRV.GetAddressOf());
    hr = dev->CreateShaderResourceView(accumATex.Get(), &sd, accumASRV.GetAddressOf());
    hr = dev->CreateShaderResourceView(accumBTex.Get(), &sd, accumBSRV.GetAddressOf());

    // RTV
    bloomARTVs.clear(); bloomBRTVs.clear();
    accumARTVs.clear(); accumBRTVs.clear();

    bloomARTVs.resize(bloomMipCount);  bloomBRTVs.resize(bloomMipCount);
    accumARTVs.resize(bloomMipCount);  accumBRTVs.resize(bloomMipCount);

    for (UINT mip = 0; mip < bloomMipCount; ++mip)
    {
        D3D11_RENDER_TARGET_VIEW_DESC rd{};
        rd.Format = DXGI_FORMAT_R11G11B10_FLOAT;
        rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rd.Texture2D.MipSlice = mip;

        hr = dev->CreateRenderTargetView(bloomATex.Get(), &rd, bloomARTVs[mip].GetAddressOf());
        hr = dev->CreateRenderTargetView(bloomBTex.Get(), &rd, bloomBRTVs[mip].GetAddressOf());
        hr = dev->CreateRenderTargetView(accumATex.Get(), &rd, accumARTVs[mip].GetAddressOf());
        hr = dev->CreateRenderTargetView(accumBTex.Get(), &rd, accumBRTVs[mip].GetAddressOf());
    }
}

void ShaderManager::CreateInputLayoutShader(const ComPtr<ID3D11Device>& dev, const ComPtr<ID3D11DeviceContext>& ctx)
{
    //---------------------------
    // 1. Skybox
    {
        // InputLayout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };

        ID3D10Blob* vertexShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/VS_Skybox.hlsl", "main", "vs_5_0", &vertexShaderBuffer));
        HR_T(dev->CreateInputLayout(layout, ARRAYSIZE(layout),
            vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &inputLayout_Position));

        // VS
        dev->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), nullptr, &VS_Skybox);
        vertexShaderBuffer->Release();

        // PS
        ID3D10Blob* pixelShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/PS_Skybox.hlsl", "main", "ps_5_0", &pixelShaderBuffer));
        HR_T(dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &PS_Skybox));
    }

    //---------------------------
    // 2. Mesh
    {
        // Input Layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {   // SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate	
            { "POSITION"    , 0, DXGI_FORMAT_R32G32B32_FLOAT  , 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL"      , 0, DXGI_FORMAT_R32G32B32_FLOAT  , 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT"     , 0, DXGI_FORMAT_R32G32B32_FLOAT  , 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BITANGENT"   , 0, DXGI_FORMAT_R32G32B32_FLOAT  , 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD"    , 0, DXGI_FORMAT_R32G32_FLOAT     , 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONE_INDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BONE_WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT , 0, 72, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        ID3D10Blob* vertexShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/VS_BaseLit_Skinned.hlsl", "main", "vs_5_0", &vertexShaderBuffer));
        HR_T(dev->CreateInputLayout(layout, ARRAYSIZE(layout),
            vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &inputLayout_Vertex));

        // VS
        HR_T(dev->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(),
            vertexShaderBuffer->GetBufferSize(), NULL, &VS_BaseLit_Model));
        SAFE_RELEASE(vertexShaderBuffer);

        // ShadowDepth_VS
        ID3D10Blob* vertexShaderBuffer3 = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/VS_ShadowDepth_Skinned.hlsl", "main", "vs_5_0", &vertexShaderBuffer3));
        HR_T(dev->CreateVertexShader(vertexShaderBuffer3->GetBufferPointer(),
            vertexShaderBuffer3->GetBufferSize(), NULL, &VS_ShadowDepth_Model));
        SAFE_RELEASE(vertexShaderBuffer3);
    }

    //---------------------------
    // 3. Particle
    {
        // Input Layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            // --- slot 0 (PER_VERTEX) : ParticleQuadVertex
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,        0, 0,  D3D11_INPUT_PER_VERTEX_DATA,   0 }, // corner
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,        0, 8,  D3D11_INPUT_PER_VERTEX_DATA,   0 }, // uv

            // --- slot 1 (PER_INSTANCE) : ParticleInstance
            { "TEXCOORD", 1, DXGI_FORMAT_R32G32B32_FLOAT,     1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // iPos
            { "TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT,           1, 12, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // iRot
            { "TEXCOORD", 3, DXGI_FORMAT_R32G32_FLOAT,        1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // iSize
            { "TEXCOORD", 4, DXGI_FORMAT_R32_FLOAT,           1, 24, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // iFrame
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  1, 28, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // iColor
        };

        ID3D10Blob* vertexShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/VS_Effect.hlsl", "main", "vs_5_0", &vertexShaderBuffer));
        HR_T(dev->CreateInputLayout(layout, ARRAYSIZE(layout),
            vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &inputLayout_Particle));

        // VS
        HR_T(dev->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(),
            vertexShaderBuffer->GetBufferSize(), NULL, &VS_Effect));
        SAFE_RELEASE(vertexShaderBuffer);

        // PS
        ID3D10Blob* pixelShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/PS_Effect.hlsl", "main", "ps_5_0", &pixelShaderBuffer));
        HR_T(dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &PS_Effect));
    }

    //---------------------------
    // Full Screen VS
    {
        ID3D10Blob* vertexShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/VS_Fullscreen.hlsl", "main", "vs_5_0", &vertexShaderBuffer));
        HR_T(dev->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(),
            vertexShaderBuffer->GetBufferSize(), NULL, &VS_FullScreen));
        SAFE_RELEASE(vertexShaderBuffer);
    }

    //---------------------------
    // LightVolume_VS
    {
        ID3D10Blob* vertexShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/VS_LightVolume.hlsl", "main", "vs_5_0", &vertexShaderBuffer));
        HR_T(dev->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(),
            vertexShaderBuffer->GetBufferSize(), NULL, &VS_LightVolume));
        SAFE_RELEASE(vertexShaderBuffer);
    }

    //---------------------------
    // PostProcess PS
    {
        ID3D10Blob* pixelShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/PS_PostProcess.hlsl", "main", "ps_5_0", &pixelShaderBuffer));
        HR_T(dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),
            pixelShaderBuffer->GetBufferSize(), NULL, &PS_PostProcess));
        SAFE_RELEASE(pixelShaderBuffer);
    }

    //---------------------------
    // ShadowDepth PS
    {
        ID3D10Blob* pixelShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/PS_ShadowDepth.hlsl", "main", "ps_5_0", &pixelShaderBuffer));
        HR_T(dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),
            pixelShaderBuffer->GetBufferSize(), NULL, &PS_ShadowDepth));
        SAFE_RELEASE(pixelShaderBuffer);
    }

    //---------------------------
    // BloomPrefilter PS
    {
        ID3D10Blob* pixelShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/PS_BloomPrefilter.hlsl", "main", "ps_5_0", &pixelShaderBuffer));
        HR_T(dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),
            pixelShaderBuffer->GetBufferSize(), NULL, &PS_BloomPrefilter));
        SAFE_RELEASE(pixelShaderBuffer);
    }

    //---------------------------
    // BloomDownsampleBlur PS
    {
        ID3D10Blob* pixelShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/PS_BloomDownsampleBlur.hlsl", "main", "ps_5_0", &pixelShaderBuffer));
        HR_T(dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),
            pixelShaderBuffer->GetBufferSize(), NULL, &PS_BloomDownsampleBlur));
        SAFE_RELEASE(pixelShaderBuffer);
    }

    //---------------------------
    // BloomUpsampleCombine PS
    {
        ID3D10Blob* pixelShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/PS_BloomUpsampleCombine.hlsl", "main", "ps_5_0", &pixelShaderBuffer));
        HR_T(dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),
            pixelShaderBuffer->GetBufferSize(), NULL, &PS_BloomUpsampleCombine));
        SAFE_RELEASE(pixelShaderBuffer);
    }

    //---------------------------
    // Gbuffer PS
    {
        ID3D10Blob* pixelShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/PS_Gbuffer.hlsl", "main", "ps_5_0", &pixelShaderBuffer));
        HR_T(dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),
            pixelShaderBuffer->GetBufferSize(), NULL, &PS_Gbuffer));
        SAFE_RELEASE(pixelShaderBuffer);
    }

    //---------------------------
    // DeferredLighting PS
    {
        ID3D10Blob* pixelShaderBuffer = nullptr;
        HR_T(CompileShaderFromFile(L"../Shaders/Woo/PS_DeferredLighting.hlsl", "main", "ps_5_0", &pixelShaderBuffer));
        HR_T(dev->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),
            pixelShaderBuffer->GetBufferSize(), NULL, &PS_DeferredLighting));
        SAFE_RELEASE(pixelShaderBuffer);
    }

}

void ShaderManager::CreateCB(const ComPtr<ID3D11Device>& dev)
{
    // 1. Frame CB
    {
        D3D11_BUFFER_DESC constBuffer_Desc = {};
        constBuffer_Desc.Usage = D3D11_USAGE_DEFAULT;
        constBuffer_Desc.ByteWidth = sizeof(FrameCB);
        constBuffer_Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constBuffer_Desc.CPUAccessFlags = 0;
        HR_T(dev->CreateBuffer(&constBuffer_Desc, nullptr, &frameCB));
    }

    // 2. TransformCB
    {
        D3D11_BUFFER_DESC constBuffer_Desc = {};
        constBuffer_Desc.Usage = D3D11_USAGE_DEFAULT;
        constBuffer_Desc.ByteWidth = sizeof(TransformCB);
        constBuffer_Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constBuffer_Desc.CPUAccessFlags = 0;
        HR_T(dev->CreateBuffer(&constBuffer_Desc, nullptr, &transformCB));
    }

    // 3. LightingCB
    {
        D3D11_BUFFER_DESC constBuffer_Desc = {};
        constBuffer_Desc.Usage = D3D11_USAGE_DEFAULT;
        constBuffer_Desc.ByteWidth = sizeof(LightingCB);
        constBuffer_Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constBuffer_Desc.CPUAccessFlags = 0;
        HR_T(dev->CreateBuffer(&constBuffer_Desc, nullptr, &lightingCB));
    }

    // 4. MaterialCB
    {
        D3D11_BUFFER_DESC constBuffer_Desc = {};
        constBuffer_Desc.Usage = D3D11_USAGE_DEFAULT;
        constBuffer_Desc.ByteWidth = sizeof(MaterialCB);
        constBuffer_Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constBuffer_Desc.CPUAccessFlags = 0;
        HR_T(dev->CreateBuffer(&constBuffer_Desc, nullptr, &materialCB));
    }

    // 5. OffsetMatrixCB
    {
        D3D11_BUFFER_DESC constBuffer_Desc = {};
        constBuffer_Desc.Usage = D3D11_USAGE_DEFAULT;
        constBuffer_Desc.ByteWidth = sizeof(OffsetMatrixCB);
        constBuffer_Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constBuffer_Desc.CPUAccessFlags = 0;
        HR_T(dev->CreateBuffer(&constBuffer_Desc, nullptr, &offsetMatrixCB));
    }

    // 6. PoseMatrixCB
    {
        D3D11_BUFFER_DESC constBuffer_Desc = {};
        constBuffer_Desc.Usage = D3D11_USAGE_DEFAULT;
        constBuffer_Desc.ByteWidth = sizeof(PoseMatrixCB);
        constBuffer_Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constBuffer_Desc.CPUAccessFlags = 0;
        HR_T(dev->CreateBuffer(&constBuffer_Desc, nullptr, &poseMatrixCB));
    }

    // 7. PostProcess CB
    {
        D3D11_BUFFER_DESC constBuffer_Desc = {};
        constBuffer_Desc.Usage = D3D11_USAGE_DEFAULT;
        constBuffer_Desc.ByteWidth = sizeof(PostProcessCB);
        constBuffer_Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constBuffer_Desc.CPUAccessFlags = 0;
        HR_T(dev->CreateBuffer(&constBuffer_Desc, nullptr, &postProcessCB));
    }

    // 8. Bloom CB
    {
        D3D11_BUFFER_DESC constBuffer_Desc = {};
        constBuffer_Desc.Usage = D3D11_USAGE_DEFAULT;
        constBuffer_Desc.ByteWidth = sizeof(BloomCB);
        constBuffer_Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constBuffer_Desc.CPUAccessFlags = 0;
        HR_T(dev->CreateBuffer(&constBuffer_Desc, nullptr, &bloomCB));
    }

    // 9. Effect CB
    {
        D3D11_BUFFER_DESC constBuffer_Desc = {};
        constBuffer_Desc.Usage = D3D11_USAGE_DEFAULT;
        constBuffer_Desc.ByteWidth = sizeof(EffectCB);
        constBuffer_Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constBuffer_Desc.CPUAccessFlags = 0;
        HR_T(dev->CreateBuffer(&constBuffer_Desc, nullptr, &effectCB));
    }
}


// [ Util Funcs ] --------------------------------------------------------------
void ShaderManager::CreateRTTex_RTV_SRV(const ComPtr<ID3D11Device>& device, int w, int h, DXGI_FORMAT fomat,
    ID3D11Texture2D** outTex, ID3D11RenderTargetView** outRTV, ID3D11ShaderResourceView** outSRV)
{
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w;
    td.Height = h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = fomat;
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;
    td.MiscFlags = 0;

    HRESULT hr = device->CreateTexture2D(&td, nullptr, outTex);
    hr = device->CreateRenderTargetView(*outTex, nullptr, outRTV);
    hr = device->CreateShaderResourceView(*outTex, nullptr, outSRV);
}

void ShaderManager::GetMipSize(UINT baseW, UINT baseH, UINT mip, UINT& outW, UINT& outH)
{
    outW = std::max<UINT>(1, baseW >> mip);     // baseW / 2^mip
    outH = std::max<UINT>(1, baseH >> mip);     // baseH / 2^mip
}

void ShaderManager::GetMipTexelSize(UINT baseW, UINT baseH, UINT mip, float& outTx, float& outTy)
{
    UINT w, h;
    GetMipSize(baseW, baseH, mip, w, h);
    outTx = 1.0f / (float)w;
    outTy = 1.0f / (float)h;
}

void ShaderManager::SetViewport(const ComPtr<ID3D11DeviceContext>& ctx, UINT width, UINT height)
{
    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = (float)std::max<UINT>(1, width);
    vp.Height = (float)std::max<UINT>(1, height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    ctx->RSSetViewports(1, &vp);
}

void ShaderManager::SetViewportForMip(const ComPtr<ID3D11DeviceContext>& ctx, UINT baseW, UINT baseH, UINT mip)
{
    UINT w, h;
    GetMipSize(baseW, baseH, mip, w, h);
    SetViewport(ctx, w, h);
}



// [ CB Getter ] --------------------------------------------------------------
ComPtr<ID3D11Buffer>& ShaderManager::GetFrameCB()
{
    return frameCB;
}

ComPtr<ID3D11Buffer>& ShaderManager::GetTransformCB()
{
    return transformCB;
}

ComPtr<ID3D11Buffer>& ShaderManager::GetLightingCB()
{
    return lightingCB;
}

ComPtr<ID3D11Buffer>& ShaderManager::GetMaterialCB()
{
    return materialCB;
}

ComPtr<ID3D11Buffer>& ShaderManager::GetOffsetMatrixCB()
{
    return offsetMatrixCB;
}

ComPtr<ID3D11Buffer>& ShaderManager::GetPoseMatrixCB()
{
    return poseMatrixCB;
}

ComPtr<ID3D11Buffer>& ShaderManager::GetPostProcessCB()
{
    return postProcessCB;
}

ComPtr<ID3D11Buffer>& ShaderManager::GetBloomCB()
{
    return bloomCB;
}

ComPtr<ID3D11Buffer>& ShaderManager::GetEffectCB()
{
    return effectCB;
}
