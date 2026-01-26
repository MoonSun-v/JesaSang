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
    context->VSSetShader(sm.VS_BaseLit_Rigid.Get(), NULL, 0);
    context->PSSetShader(sm.PS_Gbuffer.Get(), NULL, 0);

    // Sampler
    context->PSSetSamplers(0, 1, sm.linearSamplerState.GetAddressOf());

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

#if _DEBUG
    ExecutePickingPass(context, queue, cam);
#endif

    // clean up
    context->OMSetRenderTargets(0, nullptr, nullptr);
}

#if _DEBUG
#include "../../EngineSystem/SceneSystem.h"

void GeometryPass::ExecutePickingPass(ComPtr<ID3D11DeviceContext>& context, RenderQueue& queue, Camera* cam)
{
    auto scene = SceneSystem::Instance().GetCurrentScene();
    auto& sm = ShaderManager::Instance();
    context->OMSetRenderTargets(1, sm.pickingRTV.GetAddressOf(), sm.depthStencilView.Get()); // 이건 다음줄에서 해제되므로 따로 해제안함
    context->PSSetShader(sm.PS_Picking.Get(), NULL, 0); // ps용 피킹 바인드 
    // vs는 이전 gbuffer vs 사용 ( 이미 바인드 되어있다고 가정하고 사용 )


    // Model Render
    PickingCB _picking{};
    D3D11_MAPPED_SUBRESOURCE mapped{};
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

        int index = scene->GetGameObjectIndex(m.objPtr);    // 인덱스 찾기
        _picking.pickID = static_cast<UINT>(index);         // 각 메쉬별 id 등록

        context->Map( 
            sm.pickingCB.Get(),
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mapped
        );
        memcpy(mapped.pData, &_picking, sizeof(PickingCB));
        context->Unmap(sm.pickingCB.Get(), 0);

        context->PSSetConstantBuffers(9, 1, sm.pickingCB.GetAddressOf());

        // IB, VB, SRV, CB -> DrawCall
        m.mesh->Draw(context);

    }

    context->PSSetShader(nullptr, 0, 0); // ps 바인드 해제
}
#endif