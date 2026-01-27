#include "ForwardTransparentPass.h"
#include "../../Manager/ShaderManager.h"
#include "../../EngineSystem/LightSystem.h"
#include "../../EngineSystem/CameraSystem.h"
#include "../../Manager/WorldManager.h"
#include "../../Object/GameObject.h"

void ForwardTransparentPass::Execute(ComPtr<ID3D11DeviceContext>& context, RenderQueue& queue, Camera* cam)
{
    auto& sm = ShaderManager::Instance();

    // RTV, DSV
    context->RSSetViewports(1, &sm.viewport_screen);
    context->OMSetRenderTargets(1, sm.sceneHDRRTV.GetAddressOf(), sm.depthStencilView.Get());
    // TODO :: depth test only, transparent queue 정렬 drawcall
    context->OMSetDepthStencilState(sm.defualtDSS.Get(), 0);        

    // IA
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(sm.inputLayout_BoneWeightVertex.Get());

    // Shader
    context->PSSetShader(sm.PS_ForwardTransparent.Get(), NULL, 0);

    // Sampler
    context->PSSetSamplers(0, 1, sm.linearSamplerState.GetAddressOf());
    context->PSSetSamplers(1, 1, sm.shadowSamplerState.GetAddressOf());
    context->PSSetSamplers(2, 1, sm.linearClamSamplerState.GetAddressOf());

    // Blend State
    float blendFactor[4] = { 0,0,0,0 }; UINT sampleMask = 0xffffffff;
    context->OMSetBlendState(sm.alphaBlendState.Get(), blendFactor, sampleMask);

    // SRV
    context->PSSetShaderResources(5, 1, sm.shadowSRV.GetAddressOf());

    // CB
    sm.transformCBData.view = XMMatrixTranspose(cam->GetView());
    sm.transformCBData.projection = XMMatrixTranspose(cam->GetProjection());
    context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

    // Render
    auto& transparentQueue = queue.GetTransparentQueue();
    for (auto& transparentItem : transparentQueue)
    {
        // CB - Transform
        if (transparentItem.modelType == ModelType::Rigid) sm.transformCBData.model = transparentItem.model.Transpose();
        else if (transparentItem.modelType == ModelType::Static) sm.transformCBData.model = Matrix::Identity.Transpose();
        sm.transformCBData.world = transparentItem.world.Transpose();
        context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

        // VS
        switch (transparentItem.modelType) {
        case ModelType::Skeletal:
        {
            context->VSSetShader(sm.VS_BaseLit_Skeletal.Get(), NULL, 0);

            // CB - Offset, Pose
            auto& boneOffset = transparentItem.offsets->boneOffset;
            auto& bonePose = transparentItem.poses->bonePose;

            for (int i = 0; i < transparentItem.boneCount; i++)
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
        transparentItem.mesh->Draw(context);
    }

    // clean up
    context->OMSetRenderTargets(0, nullptr, nullptr);
    context->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    context->PSSetShaderResources(5, 1, nullSRV);  // shadowMap
}