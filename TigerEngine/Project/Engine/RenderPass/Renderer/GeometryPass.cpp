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
    context->IASetInputLayout(sm.inputLayout_BoneWeightVertex.Get());

    // Shader
    context->PSSetShader(sm.PS_Gbuffer.Get(), NULL, 0);

    // Sampler
    context->PSSetSamplers(0, 1, sm.linearSamplerState.GetAddressOf());

    // CB
    sm.transformCBData.view = XMMatrixTranspose(cam->GetView());
    sm.transformCBData.projection = XMMatrixTranspose(cam->GetProjection());
    context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

    // Render
    auto& opaqueQueue = queue.GetOpaqueQueue();
    for (auto& opaqueItem : opaqueQueue)
    {
        // CB - Transform
        if (opaqueItem.modelType == ModelType::Rigid) sm.transformCBData.model = opaqueItem.model.Transpose();
        else if (opaqueItem.modelType == ModelType::Static) sm.transformCBData.model = Matrix::Identity.Transpose();
        sm.transformCBData.world = opaqueItem.world.Transpose();
        context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

        // VS
        switch (opaqueItem.modelType) {
            case ModelType::Skeletal:
            {
                context->VSSetShader(sm.VS_BaseLit_Skeletal.Get(), NULL, 0);

                // CB - Offset, Pose
                auto& boneOffset = opaqueItem.offsets->boneOffset;
                auto& bonePose = opaqueItem.poses->bonePose;

                for (int i = 0; i < opaqueItem.boneCount; i++)
                {
                    sm.offsetMatrixCBData.boneOffset[i] = boneOffset[i];
                    sm.poseMatrixCBData.bonePose[i] = bonePose[i];
                }
                context->UpdateSubresource(sm.offsetMatrixCB.Get(), 0, nullptr, &sm.offsetMatrixCBData, 0, 0);
                context->UpdateSubresource(sm.poseMatrixCB.Get(), 0, nullptr, &sm.poseMatrixCBData, 0, 0);
                break;
            }
            case ModelType::Rigid:
            case ModelType::Static:
            {
                context->VSSetShader(sm.VS_BaseLit_Rigid.Get(), NULL, 0);
                break;
            }
        }

        // IB, VB, SRV, CB -> DrawCall
        opaqueItem.mesh->Draw(context);
    }

    // clean up
    context->OMSetRenderTargets(0, nullptr, nullptr);
}