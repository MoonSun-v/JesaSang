#include "DebugDrawPass.h"
#include "../Util/DebugDraw.h"
#include "../Manager/ShaderManager.h"

using namespace DebugDraw;

void DebugDrawPass::Init(const ComPtr<ID3D11Device> &device, const ComPtr<ID3D11DeviceContext> &deviceContext)
{
    DebugDraw::Initialize(device, deviceContext);
}

void DebugDrawPass::Execute(ComPtr<ID3D11DeviceContext>& context,
    RenderQueue& queue,
    Camera* cam)
{
    // 렌더타겟 다시 설정 (ImGui가 변경했을 수 있음)
    auto& sm = ShaderManager::Instance();
	context->OMSetRenderTargets(1, sm.backBufferRTV.GetAddressOf(), sm.depthStencilReadOnlyView.Get());

	// DebugDraw의 BasicEffect 설정
	DebugDraw::g_BatchEffect->SetWorld(Matrix::Identity);
	DebugDraw::g_BatchEffect->SetView(cam->GetView());
	DebugDraw::g_BatchEffect->SetProjection(cam->GetProjection());
	DebugDraw::g_BatchEffect->Apply(context.Get());

	// InputLayout 설정
	context->IASetInputLayout(DebugDraw::g_pBatchInputLayout.Get());
    
	// 블렌드 스테이트 설정 (깊이 테스트 활성화)
	context->OMSetBlendState(DebugDraw::g_States->AlphaBlend(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(DebugDraw::g_States->DepthRead(), 0);
	context->RSSetState(DebugDraw::g_States->CullNone());
}

void DebugDrawPass::End(ComPtr<ID3D11DeviceContext> &context)
{    
	// 렌더 스테이트 복원
	context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(nullptr, 0);
	context->RSSetState(nullptr);
}