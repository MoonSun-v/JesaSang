#include "ShadowPass.h"
#include "../../Manager/ShaderManager.h"
#include "../../EngineSystem/CameraSystem.h"

void ShadowPass::Execute(ComPtr<ID3D11DeviceContext>& context, RenderQueue& queue, Camera* cam)
{
    auto& sm = ShaderManager::Instance();

    // RTV, DSV
    context->RSSetViewports(1, &sm.viewport_shadowMap);
    context->OMSetRenderTargets(0, nullptr, sm.shadowDSV.Get());
    context->OMSetDepthStencilState(sm.defualtDSS.Get(), 0);
    context->ClearDepthStencilView(sm.shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    // IA
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(sm.inputLayout_BoneWeightVertex.Get());

    // Shader
    context->PSSetShader(sm.PS_ShadowDepth.Get(), NULL, 0);    // alpha discard

    // Sampler
    context->PSSetSamplers(0, 1, sm.linearSamplerState.GetAddressOf());

    // CB
    auto lightCamera = CameraSystem::Instance().lightCamera;
    auto view = lightCamera->GetView();
    auto projection = lightCamera->GetProjection();
    sm.transformCBData.shadowView = XMMatrixTranspose(view);
    sm.transformCBData.shadowProjection = XMMatrixTranspose(projection);
    context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

    // Render
    auto& models = queue.GetOpaqueQueue();
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
                context->VSSetShader(sm.VS_ShadowDepth_Skeletal.Get(), NULL, 0);

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
                context->VSSetShader(sm.VS_ShadowDepth_Rigid.Get(), NULL, 0);
                break;
            }
        }

        // IB, VB, SRV, CB -> DrawCall
        m.mesh->Draw(context);
    }
}