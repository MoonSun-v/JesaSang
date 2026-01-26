#include "ForwardTransparentPass.h"
#include "../../Manager/ShaderManager.h"


void ForwardTransparentPass::Execute(ComPtr<ID3D11DeviceContext>& context, RenderQueue& queue, Camera* cam)
{
    auto& sm = ShaderManager::Instance();

    // RTV, DSV
    context->RSSetViewports(1, &sm.viewport_screen);
    context->OMSetRenderTargets(1, sm.sceneHDRRTV.GetAddressOf(), sm.depthStencilView.Get());
    context->OMSetDepthStencilState(sm.defualtDSS.Get(), 0);

    // IA
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(sm.inputLayout_BoneWeightVertex.Get());

    // Shader
    context->PSSetShader(nullptr, NULL, 0);       // TODO :: Forward Rendering PS 작성

    // Sampler
    context->PSSetSamplers(0, 1, sm.linearSamplerState.GetAddressOf());
    context->PSSetSamplers(1, 1, sm.shadowSamplerState.GetAddressOf());
    context->PSSetSamplers(2, 1, sm.linearClamSamplerState.GetAddressOf());

    // CB
    sm.transformCBData.view = XMMatrixTranspose(cam->GetView());
    sm.transformCBData.projection = XMMatrixTranspose(cam->GetProjection());
    context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

    // Render
    auto& models = queue.GetRendertems();
    for (auto& m : models)
    {
        // CB - Transform
        if (m.modelType == ModelType::Rigid) sm.transformCBData.model = m.model.Transpose();
        else if (m.modelType == ModelType::Static) sm.transformCBData.model = Matrix::Identity.Transpose();
        sm.transformCBData.world = m.world.Transpose();
        context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

        // VS
        switch (m.modelType) {
        case ModelType::Skeletal:
        {
            context->VSSetShader(sm.VS_BaseLit_Skeletal.Get(), NULL, 0);

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
        m.mesh->Draw(context);
    }

    // clean up
    context->OMSetRenderTargets(0, nullptr, nullptr);
}