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
    context->ClearDepthStencilView(sm.depthStencilView.Get(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);     // depth, stecil clear (stecil write -> geometry, lighting pass)

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

    // stencil Ref
    const UINT groundStencilRef = 0x01;
    const UINT noneStencilRef = 0x00;

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

        // DSS (보이는 Ground만 Mask)
        if(opaqueItem.isGround)
            context->OMSetDepthStencilState(sm.groundDrawDSS.Get(), groundStencilRef);
        else
            context->OMSetDepthStencilState(sm.groundDrawDSS.Get(), noneStencilRef);

        // IB, VB, SRV, CB -> DrawCall
        opaqueItem.mesh->Draw(context);
    }

#if _DEBUG
    ExecutePickingPass(context, queue, cam);
#endif

    // clean up
    ID3D11RenderTargetView* nullRTV[4] = { nullptr, nullptr, nullptr, nullptr };
    context->OMSetRenderTargets(4, nullRTV, nullptr);
}

#if _DEBUG
#include "../../EngineSystem/SceneSystem.h"

void GeometryPass::ExecutePickingPass(ComPtr<ID3D11DeviceContext>& context, RenderQueue& queue, Camera* cam)
{
    // 이전 패스 RTV와 ViewPort 저장 ( 복원하기 위해서 )
    ID3D11RenderTargetView* prevRTV[8] = {};
    ID3D11DepthStencilView* prevDSV = nullptr;
    context->OMGetRenderTargets(8, prevRTV, &prevDSV);

    D3D11_VIEWPORT prevVP[16];
    UINT prevVPCount = 16;
    context->RSGetViewports(&prevVPCount, prevVP);

    auto scene = SceneSystem::Instance().GetCurrentScene();
    auto& sm = ShaderManager::Instance();

    // Set picking states
    context->RSSetViewports(1, &sm.viewport_screen);
    context->OMSetRenderTargets(1, sm.pickingRTV.GetAddressOf(), sm.pickingDSV.Get());

    // clear picking targets only
    const float clearColor[4] = { 0, 0, 0, 0 };
    context->ClearRenderTargetView(sm.pickingRTV.Get(), clearColor);
    context->ClearDepthStencilView(sm.pickingDSV.Get(),
        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    context->PSSetShader(sm.PS_Picking.Get(), NULL, 0); // ps용 피킹 바인드 
    // vs는 이전 gbuffer vs 사용 ( 이미 바인드 되어있다고 가정하고 사용 )

    // Draw
    PickingCB _picking{};
    D3D11_MAPPED_SUBRESOURCE mapped{};
    auto& models = queue.GetOpaqueQueue();
    context->PSSetConstantBuffers(9, 1, sm.pickingCB.GetAddressOf());

    std::vector<RenderItem> opaqueQueue = queue.GetOpaqueQueue();
    std::vector<RenderItem> transparentQueue = queue.GetTransparentQueue();

    std::vector<RenderItem*> mergedQueue;
    mergedQueue.reserve(opaqueQueue.size() + transparentQueue.size());

    for (auto& item : opaqueQueue)
        mergedQueue.push_back(&item);
    for (auto& item : transparentQueue)
        mergedQueue.push_back(&item);

    for (auto& item : mergedQueue)
    {
        // CB - Transform
        if (item->modelType == ModelType::Rigid) sm.transformCBData.model = item->model.Transpose();
        else if (item->modelType == ModelType::Static) sm.transformCBData.model = Matrix::Identity.Transpose();
        sm.transformCBData.world = item->world.Transpose();
        context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

        // vs
        switch (item->modelType) {
        case ModelType::Skeletal:
        {
            context->VSSetShader(sm.VS_BaseLit_Skeletal.Get(), NULL, 0);

            // CB - Offset, Pose
            auto& boneOffset = item->offsets->boneOffset;
            auto& bonePose = item->poses->bonePose;

            for (int i = 0; i < item->boneCount; i++)
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

        int index = scene->GetGameObjectIndex(item->objPtr);    // 인덱스 찾기
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

        // IB, VB, SRV, CB -> DrawCall
        item->mesh->Draw(context);
    }

    // 복원
    context->OMSetRenderTargets(8, prevRTV, prevDSV);
    context->RSSetViewports(prevVPCount, prevVP);

    ID3D11RenderTargetView* gbuffers[] =
    {
        sm.albedoRTV.Get(),
        sm.normalRTV.Get(),
        sm.metalRoughRTV.Get(),
        sm.emissiveRTV.Get()
    };

    context->OMSetRenderTargets(4, gbuffers, sm.depthStencilView.Get());
    for (auto* rtv : prevRTV) if (rtv) rtv->Release();
    if (prevDSV) prevDSV->Release();

    context->PSSetShader(nullptr, 0, 0); // ps 바인드 해제
}
#endif

