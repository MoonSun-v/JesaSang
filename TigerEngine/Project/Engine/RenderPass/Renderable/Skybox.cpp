#include "SkyBox.h"
#include "../../../Base/Datas/Vertex.h"
#include <Directxtk/DDSTextureLoader.h>

using namespace DirectX;

void SkyBox::Create(const ComPtr<ID3D11Device>& device, const wchar_t* skyboxPath,
    const wchar_t* irradiPath, const wchar_t* envPath, const wchar_t* lutPath)
{
    // Vertex Buffer, Index Buffer
    Position_Vertex vertices[] =
    {
        { Vector3(-1.0f,  1.0f, -1.0f) }, // 0
        { Vector3(1.0f,  1.0f, -1.0f) }, // 1
        { Vector3(1.0f, -1.0f, -1.0f) }, // 2
        { Vector3(-1.0f, -1.0f, -1.0f) }, // 3
        { Vector3(-1.0f,  1.0f,  1.0f) }, // 4
        { Vector3(1.0f,  1.0f,  1.0f) }, // 5
        { Vector3(1.0f, -1.0f,  1.0f) }, // 6
        { Vector3(-1.0f, -1.0f,  1.0f) }  // 7
    };

    UINT indices[] =
    {
        0,1,2, 0,2,3, // back
        4,6,5, 4,7,6, // front
        4,5,1, 4,1,0, // top
        3,2,6, 3,6,7, // bottom
        1,5,6, 1,6,2, // right
        4,0,3, 4,3,7  // left
    };

    indexCount = ARRAYSIZE(indices);

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;
    device->CreateBuffer(&vbDesc, &vbData, &vertexBuffer);
    vertexBufferStride = sizeof(Position_Vertex);
    vertexBufferOffset = 0;

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices;
    device->CreateBuffer(&ibDesc, &ibData, &indexBuffer);

    // CubeMap Texture Load
    DirectX::CreateDDSTextureFromFileEx(
        device.Get(),
        nullptr,
        skyboxPath,
        0,
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE,
        0,
        0,
        DirectX::DDS_LOADER_FORCE_SRGB,          // SRGB
        nullptr,
        &skyboxSRV,
        nullptr
    );

    // IBL Texture Load
    CreateDDSTextureFromFile(device.Get(), irradiPath, nullptr, ibl_irradianceSRV.GetAddressOf());
    CreateDDSTextureFromFile(device.Get(), envPath, nullptr, ibl_specularEnvSRV.GetAddressOf());
    CreateDDSTextureFromFile(device.Get(), lutPath, nullptr, ibl_brdfLutSRV.GetAddressOf());
}

void SkyBox::Draw(ComPtr<ID3D11DeviceContext>& context) const
{
    // VB, IB
    context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &vertexBufferStride, &vertexBufferOffset);
    context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    // SRV
    context->PSSetShaderResources(15, 1, skyboxSRV.GetAddressOf());
    context->PSSetShaderResources(11, 1, ibl_irradianceSRV.GetAddressOf());
    context->PSSetShaderResources(12, 1, ibl_specularEnvSRV.GetAddressOf());
    context->PSSetShaderResources(13, 1, ibl_brdfLutSRV.GetAddressOf());

    // Draw
    context->DrawIndexed(indexCount, 0, 0);
}