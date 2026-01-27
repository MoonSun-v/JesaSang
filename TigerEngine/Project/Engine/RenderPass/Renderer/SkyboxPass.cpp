#include "SkyboxPass.h"
#include "../Renderable/Skybox.h"
#include "../../Manager/ShaderManager.h"

SkyboxPass::SkyboxPass()
{
    skybox = new SkyBox();
}

SkyboxPass::~SkyboxPass()
{
    if (skybox) delete skybox;
}

void SkyboxPass::Init(const ComPtr<ID3D11Device>& device)
{
    skybox->Create(device, L"..\\Assets\\Resource\\Skybox\\redskyEnvHDR.dds",
        L"..\\Assets\\Resource\\Skybox\\IBL\\redskyDiffuseHDR.dds",
        L"..\\Assets\\Resource\\Skybox\\IBL\\redskySpecularHDR.dds",
        L"..\\Assets\\Resource\\Skybox\\IBL\\redskyBrdf.dds");
}


// [ Skybox Render ]
// Deferred 렌더링에서 스카이박스는 Lighting Pass 이후에 렌더링되며
// 비어있는 픽셀에 기록한다. (Depth Test)
void SkyboxPass::Execute(ComPtr<ID3D11DeviceContext>& context, RenderQueue& queue, Camera* cam)
{
    auto& sm = ShaderManager::Instance();

    // RTV, DSV
    context->RSSetViewports(1, &sm.viewport_screen);
    context->OMSetRenderTargets(1, sm.sceneHDRRTV.GetAddressOf(), sm.depthStencilView.Get());
    context->OMSetDepthStencilState(sm.depthTestOnlyDSS.Get(), 0);

    // IA
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(sm.inputLayout_Position.Get());

    // Shader
    context->VSSetShader(sm.VS_Skybox.Get(), nullptr, 0);
    context->PSSetShader(sm.PS_Skybox.Get(), nullptr, 0);

    // Sampler
    context->PSSetSamplers(0, 1, sm.linearSamplerState.GetAddressOf());

    // RS, DSS
    context->RSSetState(sm.cullfrontRS.Get());
    context->OMSetDepthStencilState(sm.depthTestOnlyDSS.Get(), 0);

    // CB
    /* 카메라 이동행렬 제거->카메라가 이동해도 큐브는 항상 카메라 원점에 고정
       스카이 박스 정점은 카메라 좌표계에서 항상 +- 1 정도 거리의 정점으로 유지
       큐브는 투영후 카메라의 Far Plane에 수렴하는 값으로 나오고 z(깊이)는 1근처가 됨 */
    Matrix viewNoTranslation = cam->GetView();
    viewNoTranslation._41 = 0.0f;
    viewNoTranslation._42 = 0.0f;
    viewNoTranslation._43 = 0.0f;

    sm.transformCBData.view = XMMatrixTranspose(viewNoTranslation);
    sm.transformCBData.projection = XMMatrixTranspose(cam->GetProjection());
    context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

    // Draw
    skybox->Draw(context);

    // view 복원
    sm.transformCBData.view = XMMatrixTranspose(cam->GetView());
    context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

    // clear
    context->RSSetState(nullptr);
    context->OMSetDepthStencilState(nullptr, 0);
}