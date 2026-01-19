#include "ShaderManager.h"
#include <Datas/Vertex.h>
#include <Datas/FBXResourceData.h>
#include "../Base/Datas/ConstantBuffer.hpp"


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
