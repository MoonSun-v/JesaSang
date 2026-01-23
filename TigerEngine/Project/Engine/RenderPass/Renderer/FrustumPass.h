#pragma once
#include "../IRenderPass.h"
#include "../../../Base/pch.h"
#include "directxtk/VertexTypes.h"        
#include "directxtk/PrimitiveBatch.h"     
#include "directxtk/CommonStates.h"
#include "directxtk/Effects.h"
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <foundation/PxTransform.h>

// Light Camera Frustum Debug Draw 전용으로 임시 추가한 렌더 패스
// Execute 코드에서 라이트캠만 찾아서 처리하고잇음

class FrustumPass : public IRenderPass
{
private:
    unique_ptr<CommonStates>  m_states;
    unique_ptr<PrimitiveBatch<VertexPositionColor>> m_batch;
    unique_ptr<BasicEffect>   m_effect;
    ComPtr<ID3D11InputLayout> m_layout = nullptr;

public:
    void Init(const ComPtr<ID3D11Device>& device, const ComPtr<ID3D11DeviceContext>& context);
    void Execute(ComPtr<ID3D11DeviceContext>& context,
        RenderQueue& queue, Camera* cam) override;
    void End(ComPtr<ID3D11DeviceContext>& context) override {}

private:
    void FrustumDebugDraw(ComPtr<ID3D11DeviceContext>& context, const Matrix& frustumView, const Matrix& frustumProj,
        const Matrix& renderView, const Matrix& renderProj, FXMVECTOR color = Colors::Red);
};

