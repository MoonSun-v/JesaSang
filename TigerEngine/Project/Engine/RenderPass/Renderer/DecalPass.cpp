#include "DecalPass.h"
#include "../Renderable/DecalVolumeMesh.h"
#include "../../Manager/ShaderManager.h"
#include "../../EngineSystem/CameraSystem.h"
#include "../../EngineSystem/DecalSystem.h"
#include "../../Object/GameObject.h"

DecalPass::~DecalPass()
{
    if (decalVolume) delete decalVolume;
}

void DecalPass::Init(const ComPtr<ID3D11Device>& device)
{
    // box volume create
    decalVolume = CreateDecalVolume(device.Get());
}

void DecalPass::Execute(ComPtr<ID3D11DeviceContext>& context, RenderQueue& queue, Camera* cam)
{
    auto& sm = ShaderManager::Instance();

    context->RSSetViewports(1, &sm.viewport_screen);
    context->OMSetRenderTargets(1, sm.albedoRTV.GetAddressOf(), sm.depthStencilReadOnlyView.Get());

    // IA
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(sm.inputLayout_Position.Get());

    // DSS - Stencil Test(ground)
    const UINT stencilRef = 0x01;          // Stencil Reference Value
    context->OMSetDepthStencilState(sm.groundTestDSS.Get(), stencilRef);

    // Shader
    context->VSSetShader(sm.VS_Decal.Get(), nullptr, 0);
    context->PSSetShader(sm.PS_Decal.Get(), nullptr, 0);

    // Sampler
    context->PSSetSamplers(0, 1, sm.linearSamplerState.GetAddressOf());
    context->PSSetSamplers(2, 1, sm.linearClamSamplerState.GetAddressOf());

    // SRV
    context->PSSetShaderResources(7, 1, sm.normalSRV.GetAddressOf());
    context->PSSetShaderResources(10, 1, sm.depthSRV.GetAddressOf());

    // Blend State
    float blendFactor[4] = { 0,0,0,0 }; UINT sampleMask = 0xffffffff;
    context->OMSetBlendState(sm.alphaBlendState.Get(), blendFactor, sampleMask);

    // Render
    auto decals = DecalSystem::Instance().GetComponents();
    for(Decal* decal : decals)
    {
        decalVolume->UpdateWolrd(decal);
        decalVolume->Draw(context, decal);
    }

    // clean up
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);

    ID3D11RenderTargetView* nullRTV[1] = { nullptr };
    context->OMSetRenderTargets(1, nullRTV, nullptr);

    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    context->PSSetShaderResources(7, 1, nullSRV);  // normal
    context->PSSetShaderResources(10, 1, nullSRV); // depth
}