#include "ShaderManager.h"
#include <Datas/Vertex.h>
#include <Datas/FBXResourceData.h>
#include "../Base/Datas/ConstantBuffer.hpp"
#include "../Base/Datas/Vertex.h"

void ShaderManager::Init(const ComPtr<ID3D11Device>& dev, const ComPtr<ID3D11DeviceContext>& ctx)
{
    CreateDSS(dev);
    CreateRS(dev);
    CreateSampler(dev);
    CreateInputLayoutShader(dev, ctx);
    CreateCB(dev);
}


// --------------------------------------------------------------
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


// --------------------------------------------------------------
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
