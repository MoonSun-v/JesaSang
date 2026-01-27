#include "DecalVolumeMesh.h"
#include "../../Components/Decal.h"
#include "../../../Base/Datas/Vertex.h"
#include "../../../Base/Datas/ConstantBuffer.hpp"
#include "../../Manager/ShaderManager.h"
#include "../../Object/GameObject.h"

DecalVolumeMesh::DecalVolumeMesh()
    : indexCount(0), stride(sizeof(Position_Vertex)),
    indexFormat(DXGI_FORMAT_R32_UINT), world(Matrix::Identity)
{
}

void DecalVolumeMesh::UpdateWolrd(Decal* decal)
{
    world = decal->GetOwner()->GetTransform()->GetWorldTransform();
}

void DecalVolumeMesh::Draw(ComPtr<ID3D11DeviceContext>&context, Decal* decal) const
{
    auto& sm = ShaderManager::Instance();

    // CB - Transform
    sm.transformCBData.world = XMMatrixTranspose(world);
    context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

    // CB - Decal
    sm.decalCBData.decalInvWorld = XMMatrixTranspose(world.Invert());
    sm.decalCBData.opacity = decal->opacity;
    sm.decalCBData.upThreshold = decal->upThreshold;
    sm.decalCBData.tiling = decal->tiling;
    sm.decalCBData.offset = decal->offset;
    context->UpdateSubresource(sm.decalCB.Get(), 0, nullptr, &sm.decalCBData, 0, 0);

    // VB, IB
    UINT offset = 0;
    context.Get()->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    context.Get()->IASetIndexBuffer(indexBuffer.Get(), indexFormat, 0);

    // SRV
    if (decal->decalSRV)
    {
        cout << "Decal Volume Draw with Decal Texture" << endl;
        context.Get()->PSSetShaderResources(19, 1, decal->decalSRV.GetAddressOf());
    }

    // Draw Call
    context.Get()->DrawIndexed(indexCount, 0, 0);
}

DecalVolumeMesh* CreateDecalVolume(ID3D11Device* device)
{
    auto* mesh = new DecalVolumeMesh();

    // 원점 기준, 한 변 길이
    const float h = 10.0f;

    // 8 vertices
    Position_Vertex vertices[8] =
    {
        { {-h, -h, -h} }, // 0
        { {-h, +h, -h} }, // 1
        { {+h, +h, -h} }, // 2
        { {+h, -h, -h} }, // 3
        { {-h, -h, +h} }, // 4
        { {-h, +h, +h} }, // 5
        { {+h, +h, +h} }, // 6
        { {+h, -h, +h} }, // 7
    };

    // 12 triangles = 36 indices
    uint32_t indices[36] =
    {
        // -Z (back)
        0, 1, 2,   0, 2, 3,

        // +Z (front)
        4, 6, 5,   4, 7, 6,

        // -X (left)
        0, 5, 1,   0, 4, 5,

        // +X (right)
        3, 2, 6,   3, 6, 7,

        // +Y (top)
        1, 5, 6,   1, 6, 2,

        // -Y (bottom)
        0, 3, 7,   0, 7, 4,
    };

    mesh->indexCount = static_cast<UINT>(_countof(indices));
    mesh->stride = sizeof(Position_Vertex);
    mesh->indexFormat = DXGI_FORMAT_R32_UINT;

    // Vertex Buffer
    {
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_IMMUTABLE;
        bd.ByteWidth = sizeof(vertices);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = vertices;

        HRESULT hr = device->CreateBuffer(&bd, &initData, mesh->vertexBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            delete mesh;
            return nullptr;
        }
    }

    // Index Buffer
    {
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_IMMUTABLE;
        bd.ByteWidth = sizeof(indices);
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = indices;

        HRESULT hr = device->CreateBuffer(&bd, &initData, mesh->indexBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            delete mesh;
            return nullptr;
        }
    }

    return mesh;
}
