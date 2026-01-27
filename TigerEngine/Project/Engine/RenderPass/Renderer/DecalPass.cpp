#include "DecalPass.h"
#include "../Renderable/DecalVolumeMesh.h"
#include "../../Manager/ShaderManager.h"
#include "../../EngineSystem/CameraSystem.h"
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

    // RTV, DSV
    ID3D11RenderTargetView* gbuffers[] =
    {
        sm.albedoRTV.Get(),
        sm.normalRTV.Get(),
        sm.metalRoughRTV.Get(),
        sm.emissiveRTV.Get()
    };

    context->RSSetViewports(1, &sm.viewport_screen);
    context->OMSetRenderTargets(4, gbuffers, sm.depthStencilView.Get());

    // IA
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(sm.inputLayout_Position.Get());

    // Shader
    context->VSSetShader(nullptr, nullptr, 0);    // TODO :: Shader 작성
    context->PSSetShader(nullptr, nullptr, 0);    // TODO :: Shader 작성

    // DSS
    const UINT stencilRef = 0x01;          // Stencil Reference Value
    context->OMSetDepthStencilState(sm.groundTestDSS.Get(), stencilRef);

    // RS
    //context.Get()->RSSetState(sm.cullfrontRS.Get());  // 확인 필요

    // Render
    // TODO :: decal Compoennt 추가 후 for each 돌면서 world update/ draw

    // clean up
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);

    ID3D11RenderTargetView* nullRTV[4] = { nullptr, nullptr, nullptr, nullptr };
    context->OMSetRenderTargets(4, nullRTV, nullptr);
}