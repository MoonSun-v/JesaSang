#include "Mesh.h"
#include "../Helper.h"
#include "../../Engine/Manager/ShaderManager.h"

void Mesh::Draw(ComPtr<ID3D11DeviceContext>& pDeviceContext) const
{
    auto& sm = ShaderManager::Instance();

    // VB, IB
    UINT stride = sizeof(BoneWeightVertexData); UINT offset = 0;
    pDeviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
    pDeviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);        
    
    // SRV
    ID3D11ShaderResourceView* nullSRVs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    pDeviceContext->PSSetShaderResources(0, 5, nullSRVs);

    int textureCount = textures.size();
    for (int i = 0; i < textureCount; i++)
    {
        ProcessTextureByType(pDeviceContext, i);
    }

    // CB - Material
    sm.materialCBData.useDiffuse = material.hasDiffuse;
    sm.materialCBData.useNormal  = material.hasNormal;
    sm.materialCBData.useEmissive = material.hasEmissive;
    sm.materialCBData.useMetallic = material.hasMetallic;
    sm.materialCBData.useRoughness = material.hasRoughness;
    sm.materialCBData.roughnessFromShininess = material.roughnessFromShininess;

    sm.materialCBData.emissiveFactor = material.emissiveFactor;
    sm.materialCBData.metallicFactor = material.metallicFactor;
    sm.materialCBData.roughnessFactor = material.roughnessFactor;

    sm.materialCBData.usediffuseOverride = material.usediffuseOverride;
    sm.materialCBData.useEmissiveOverride = material.useEmissiveOverride;
    sm.materialCBData.useMetallicOverride = material.useMetallicOverride;
    sm.materialCBData.useRoughnessOverride = material.useRoughnessOverride;

    sm.materialCBData.diffuseOverride = material.diffuseOverride;
    sm.materialCBData.metallicOverride = material.metallicOverride;
    sm.materialCBData.emissiveOverride = material.emissiveOverride;
    sm.materialCBData.roughnessOverride = material.roughnessOverride;
    pDeviceContext->UpdateSubresource(sm.materialCB.Get(), 0, nullptr, &sm.materialCBData, 0, 0);
    
    // Draw Call
    pDeviceContext->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
}

void Mesh::SetMaterial(aiMaterial* pAiMaterial)
{
    //aiColor4D color(0, 0, 0, 0);
    //if (AI_SUCCESS == pAiMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color)) //
    //{
    //    material.diffuseOverride = { color.r, color.g, color.b, color.a };
    //}
}

void Mesh::setupMesh()
{
    // check texture
    for (auto& tex : textures)
    {
        string typeName = tex.type;
        if (typeName == TEXTURE_DIFFUSE)
        {
            material.hasDiffuse = true;
        }
        else if (typeName == TEXTURE_EMISSIVE)
        {
            material.hasEmissive = true;
        }
        else if (typeName == TEXTURE_NORMAL)
        {
            material.hasNormal = true;
        }
        else if (typeName == TEXTURE_METALNESS)
        {
            material.hasMetallic = true;
        }
        else if (typeName == TEXTURE_ROUGHNESS)
        {
            material.hasRoughness= true;

        }
        else if (typeName == TEXTURE_SHININESS)
        {
            material.hasRoughness = true;
            material.roughnessFromShininess = true;
        }
        //else if (typeName == TEXTURE_SPECULAR)
        //{
        //    PBR Has None Specular
        //}
    }
}

void Mesh::ProcessTextureByType(ComPtr<ID3D11DeviceContext>& pDeviceContext, int index) const
{
    string typeName = textures[index].type;

    if (typeName == TEXTURE_DIFFUSE)
    {
        pDeviceContext->PSSetShaderResources(0, 1, textures[index].pTexture.GetAddressOf());
    }
    else if (typeName == TEXTURE_NORMAL)
    {
        pDeviceContext->PSSetShaderResources(1, 1, textures[index].pTexture.GetAddressOf());
    }
    else if (typeName == TEXTURE_METALNESS)
    {
        pDeviceContext->PSSetShaderResources(2, 1, textures[index].pTexture.GetAddressOf());
    }
    else if (typeName == TEXTURE_ROUGHNESS)
    {
        pDeviceContext->PSSetShaderResources(3, 1, textures[index].pTexture.GetAddressOf());
    }
    else if (typeName == TEXTURE_SHININESS)
    {
        pDeviceContext->PSSetShaderResources(3, 1, textures[index].pTexture.GetAddressOf());
    }
    else if (typeName == TEXTURE_EMISSIVE)
    {
        pDeviceContext->PSSetShaderResources(4, 1, textures[index].pTexture.GetAddressOf());
    }
    
    //else if (typeName == TEXTURE_SPECULAR)
    //{
    //    PBR Has None Specular
    //    pDeviceContext->PSSetShaderResources(3, 1, textures[index].pTexture.GetAddressOf());
    //}
}

Material& Mesh::GetMaterial()
{
    return material;
}

void Mesh::CreateVertexBuffer(ComPtr<ID3D11Device>& dev)
{
    // vertex buffer
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = static_cast<UINT>(sizeof(BoneWeightVertexData) * vertices.size());
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = &vertices[0];

    HR_T(dev->CreateBuffer(&vbd, &initData, m_pVertexBuffer.GetAddressOf()));
}

void Mesh::CreateIndexBuffer(ComPtr<ID3D11Device>& dev)
{
    // index buffer
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = static_cast<UINT>(sizeof(UINT) * indices.size());
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = &indices[0];
    HR_T(dev->CreateBuffer(&ibd, &initData, m_pIndexBuffer.GetAddressOf()));
}

//void Mesh::CreateMaterialBuffer(ComPtr<ID3D11Device> &dev)
//{
//    // material buffer
//    D3D11_BUFFER_DESC mbd = {};
//    mbd.Usage = D3D11_USAGE_DEFAULT;
//    mbd.ByteWidth = sizeof(MaterialData);
//    mbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
//    mbd.CPUAccessFlags = 0;
//    HR_T(dev->CreateBuffer(&mbd, nullptr, materialBuffer.GetAddressOf()));
//}
