#include "FrustumPass.h"
#include "../../Util/DebugDraw.h"
#include "../../EngineSystem/CameraSystem.h"

void FrustumPass::Init(const ComPtr<ID3D11Device>& device, const ComPtr<ID3D11DeviceContext>& context)
{
    m_states = std::make_unique<CommonStates>(device.Get());
    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context.Get());
    m_effect = std::make_unique<BasicEffect>((device.Get()));
    m_effect->SetVertexColorEnabled(true);

    {
        void const* shaderByteCode;
        size_t byteCodeLength;

        m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        device.Get()->CreateInputLayout(
            VertexPositionColor::InputElements, VertexPositionColor::InputElementCount,
            shaderByteCode, byteCodeLength,
            m_layout.ReleaseAndGetAddressOf());
    }
}

void FrustumPass::Execute(ComPtr<ID3D11DeviceContext>& context, RenderQueue& queue, Camera* cam)
{
    auto lightCam = CameraSystem::Instance().lightCamera;
    auto shadowView = lightCam->GetView();
    auto shadowProjection = lightCam->GetProjection();

    auto view = cam->GetView();
    auto projection = cam->GetProjection();

    FrustumDebugDraw(context, shadowView, shadowProjection, 
        view, projection, Colors::Red);
}

void FrustumPass::FrustumDebugDraw(ComPtr<ID3D11DeviceContext>& context, const Matrix& frustumView, const Matrix& frustumProj, const Matrix& renderView, const Matrix& renderProj, FXMVECTOR color)
{
    // Frustum Create
    BoundingFrustum frustum{};
    BoundingFrustum::CreateFromMatrix(frustum, frustumProj); // view space 기준
    Matrix invFrustumView = frustumView.Invert();
    frustum.Transform(frustum, invFrustumView);        // view -> world

    // Effect Update (render 기준은 항상 main camera)
    m_effect->SetWorld(Matrix::Identity);
    m_effect->SetView(renderView);
    m_effect->SetProjection(renderProj);
    m_effect->Apply(context.Get());

    // Stage Setting
    context.Get()->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context.Get()->OMSetDepthStencilState(m_states->DepthNone(), 0);
    context.Get()->RSSetState(m_states->CullNone());
    context.Get()->IASetInputLayout(m_layout.Get());

    // Draw
    m_batch->Begin();
    DebugDraw::Draw(m_batch.get(), frustum, color);
    m_batch->End();

    // UnBind
    const float blendFactor[4] = { 0,0,0,0 };
    context.Get()->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
    context.Get()->OMSetDepthStencilState(nullptr, 0);
    context.Get()->RSSetState(nullptr);
}
