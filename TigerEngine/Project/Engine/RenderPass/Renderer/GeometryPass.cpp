#include "GeometryPass.h"
#include "../../Manager/ShaderManager.h"


void GeometryPass::Execute(ComPtr<ID3D11DeviceContext>& context, RenderQueue& queue, Camera* cam)
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
    context->ClearDepthStencilView(sm.depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    context->OMSetDepthStencilState(sm.defualtDSS.Get(), 0);

    for (int i = 0; i < 4; i++)
    {
        context->ClearRenderTargetView(gbuffers[i], clearColor);
    }

    // IA
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(sm.inputLayout_Vertex.Get());

    // Shader
    context->VSSetShader(sm.VS_BaseLit_Model.Get(), NULL, 0);
    context->PSSetShader(sm.PS_Gbuffer.Get(), NULL, 0);

    // Sampler
    context->PSSetSamplers(0, 1, sm.linearSamplerState.GetAddressOf());

    // CB
    sm.transformCBData.view = XMMatrixTranspose(cam->GetView());
    sm.transformCBData.projection = XMMatrixTranspose(cam->GetProjection());
    context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

    // Render
    auto& models = queue.GetSkeletaItems();
    for (auto& m : models)
    {
        // CB - Transform
        sm.transformCBData.world = m.world.Transpose();
        sm.transformCBData.isRigid = m.isRigid;
        sm.transformCBData.refBoneIndex = m.refBoneIndex;
        context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

        // CB - Offset, Pose
        auto& boneOffset = m.offsets->boneOffset;
        auto& bonePose = m.poses->bonePose;

        for (int i = 0; i < m.boneCount; i++)
        {
            sm.offsetMatrixCBData.boneOffset[i] = boneOffset[i];
            sm.poseMatrixCBData.bonePose[i] = bonePose[i];
        }
        context->UpdateSubresource(sm.offsetMatrixCB.Get(), 0, nullptr, &sm.offsetMatrixCBData, 0, 0);
        context->UpdateSubresource(sm.poseMatrixCB.Get(), 0, nullptr, &sm.poseMatrixCBData, 0, 0);

        // IB, VB, SRV, CB -> DrawCall
        m.mesh->Draw(context);
    }

    // clean up
    context->OMSetRenderTargets(0, nullptr, nullptr);
}