#include "LightPass.h"
#include "../Renderable/LightvolumeMesh.h"
#include "../../EngineSystem/LightSystem.h"
#include "../../EngineSystem/CameraSystem.h"
#include "../../Manager/ShaderManager.h"
#include "../../Manager/WorldManager.h"
#include "../../Object/GameObject.h"

using namespace std;

LightPass::~LightPass()
{
    if (sphereVolume) delete sphereVolume;
    if (coneVolume) delete coneVolume;
}

void LightPass::Init(const ComPtr<ID3D11Device>& device)
{
    // light volume create
    sphereVolume = CreateLightVolumeSphere(device.Get(), 24, 16);
    coneVolume = CreateLightVolumeCone(device.Get(), 24, false);
}


void LightPass::Execute(ComPtr<ID3D11DeviceContext>& context, RenderQueue& queue, Camera* cam)
{
    StencilPass(context, cam);
    LightingPass(context, cam);
}

// [ Stencil Pass ]
//  Lighting Volume을 그리며 Stencil Buffer에 라이팅 연산 영역 마크
//  라이팅 연산 영역이란 ? 라이팅 볼륨 안의 픽셀중 G-Buffer의 깊이값보다 가까운 픽셀
//  RTV는 바인딩 하지 않고 Stecnil Buffer만 사용한다.
void LightPass::StencilPass(ComPtr<ID3D11DeviceContext>& context, Camera* camera)
{
    auto& sm = ShaderManager::Instance();
    std::vector<Light*> lights = LightSystem::Instance().GetComponents();

    // RTV, DSV (Stencil)
    context->RSSetViewports(1, &sm.viewport_screen);
    context->OMSetRenderTargets(0, nullptr, sm.depthStencilView.Get());

    // Stencil Clear를 Geometry Pass로 옮김
    // -> Ground Mesh를 그릴때 Stecil을 쓸거기 때문에 clear 방지
    //context->ClearDepthStencilView(sm.depthStencilView.Get(),
    //    D3D11_CLEAR_STENCIL, 1.0f, 0);  // Stencil만 0으로 초기화

    // IA
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(sm.inputLayout_Position.Get());

    // Shader
    context->VSSetShader(sm.VS_LightVolume.Get(), nullptr, 0);
    context->PSSetShader(nullptr, nullptr, 0);   // PS x

    // DSS
    const UINT stencilRef = 0x02;          // Stencil Reference Value
    context->OMSetDepthStencilState(sm.lightingVolumeDrawDSS.Get(), stencilRef);

    // RS (원래 outside는 cullBack인데, mesh가 뒤집혀있는듯?)
    context.Get()->RSSetState(sm.cullfrontRS.Get());

    // Render
    for (Light*& light : lights)
    {
        if (light->type == LightType::Point)
        {
            sphereVolume->UpdateWolrd(*light);
            sphereVolume->Draw(context, *camera);
        }
        else if (light->type == LightType::Spot)
        {
            coneVolume->UpdateWolrd(*light);
            coneVolume->Draw(context, *camera);
        }
    }

    // clear
    context->OMSetDepthStencilState(nullptr, 0);
    context->RSSetState(nullptr);
}


// [ Lighting Pass ]
//  G-Buffer를 샘플링하여 라이팅 계산
//  - Directional : Full Screen Quad
//  - Point, Spot : Light Volume + Stencil Test
void LightPass::LightingPass(ComPtr<ID3D11DeviceContext>& context, Camera* camera)
{
    auto& sm = ShaderManager::Instance();
    std::vector<Light*> lights = LightSystem::Instance().GetComponents();
    Vector3 camPos = camera->GetOwner()->GetTransform()->GetPosition();

    // RTV, DSV
    context->RSSetViewports(1, &sm.viewport_screen);
    context->OMSetRenderTargets(1, sm.sceneHDRRTV.GetAddressOf(), sm.depthStencilReadOnlyView.Get());
    context->ClearRenderTargetView(sm.sceneHDRRTV.Get(), clearColor);

    // IA
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // PS
    context->PSSetShader(sm.PS_DeferredLighting.Get(), NULL, 0);

    // Sampler
    context->PSSetSamplers(0, 1, sm.linearSamplerState.GetAddressOf());
    context->PSSetSamplers(1, 1, sm.shadowSamplerState.GetAddressOf());
    context->PSSetSamplers(2, 1, sm.linearClamSamplerState.GetAddressOf());

    // Blend State (Additive)
    context->OMSetBlendState(sm.additiveBlendState.Get(), nullptr, 0xffffffff);

    // SRV
    context->PSSetShaderResources(5, 1, sm.shadowSRV.GetAddressOf());
    context->PSSetShaderResources(6, 1, sm.albedoSRV.GetAddressOf());
    context->PSSetShaderResources(7, 1, sm.normalSRV.GetAddressOf());
    context->PSSetShaderResources(8, 1, sm.metalRoughSRV.GetAddressOf());
    context->PSSetShaderResources(9, 1, sm.emissiveSRV.GetAddressOf());
    context->PSSetShaderResources(10, 1, sm.depthSRV.GetAddressOf());

    // CB - Transform
    auto lightCamera = CameraSystem::Instance().lightCamera;
    sm.transformCBData.shadowView = XMMatrixTranspose(lightCamera->GetView());
    sm.transformCBData.shadowProjection = XMMatrixTranspose(lightCamera->GetProjection());
    XMMATRIX invVP = XMMatrixInverse(nullptr, camera->GetView() * camera->GetProjection());
    sm.transformCBData.invViewProjection = XMMatrixTranspose(invVP);
    context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

    // CB - Light
    auto& wm = WorldManager::Instance();
    sm.lightingCBData.useIBL = wm.useIBL;
    sm.lightingCBData.indirectIntensity = wm.indirectIntensity;
    context->UpdateSubresource(sm.lightingCB.Get(), 0, nullptr, &sm.lightingCBData, 0, 0);

    // Render
    for (Light*& light : lights)
    {
       auto lightPos = light->GetOwner()->GetTransform()->GetPosition();

       // CB - Light
       sm.lightingCBData.lightType = static_cast<int>(light->type);
       sm.lightingCBData.isSunLight = light->isSunLight;
       sm.lightingCBData.lightColor = light->color;
       sm.lightingCBData.directIntensity = light->intensity;
       sm.lightingCBData.lightDirection = light->direction;
       sm.lightingCBData.lightPos = lightPos;
       sm.lightingCBData.lightRange = light->range;
       sm.lightingCBData.innerAngle = light->innerAngle;
       sm.lightingCBData.outerAngle = light->outerAngle;
       context->UpdateSubresource(sm.lightingCB.Get(), 0, nullptr, &sm.lightingCBData, 0, 0);

        // Light Volume 렌더링
        if (light->type == LightType::Directional)
        {
            // Stencil Test off
            context->OMSetDepthStencilState(sm.disableDSS.Get(), 0);

            // RS
            context.Get()->RSSetState(nullptr);

            // Full Screen Quad
            context->IASetInputLayout(nullptr);
            context->VSSetShader(sm.VS_FullScreen.Get(), NULL, 0);
            context.Get()->Draw(3, 0);
        }
        else
        {
            // Light Volume
            if (light->type == LightType::Point)
            {
                if (sphereVolume->IsInsidePointLight(camPos, lightPos, light->range))
                {
                    // Stencil Test off
                    context->OMSetDepthStencilState(sm.disableDSS.Get(), 0);

                    // RS
                    context.Get()->RSSetState(nullptr);

                    // Full Screen Quad
                    context->IASetInputLayout(nullptr);
                    context->VSSetShader(sm.VS_FullScreen.Get(), NULL, 0);
                    context.Get()->Draw(3, 0);
                }
                else
                {
                    // Stencil Test on
                    context->OMSetDepthStencilState(sm.lightingVolumeTestDSS.Get(), 1);

                    // RS
                    context.Get()->RSSetState(sm.cullfrontRS.Get());

                    // Light Volume
                    context.Get()->IASetInputLayout(sm.inputLayout_Position.Get());
                    context.Get()->VSSetShader(sm.VS_LightVolume.Get(), nullptr, 0);
                    sphereVolume->UpdateWolrd(*light);
                    sphereVolume->Draw(context, *camera);
                }
            }
            else if (light->type == LightType::Spot)
            {
                if (sphereVolume->IsInsideSpotLight(camPos, lightPos,
                    light->direction, light->range, light->outerAngle))
                {
                    // Stencil Test off
                    context->OMSetDepthStencilState(sm.disableDSS.Get(), 0);

                    // RS
                    context.Get()->RSSetState(nullptr);

                    // Full Screen Quad
                    context->IASetInputLayout(nullptr);
                    context->VSSetShader(sm.VS_FullScreen.Get(), NULL, 0);
                    context.Get()->Draw(3, 0);
                }
                else
                {
                    // Stencil Test on
                    context->OMSetDepthStencilState(sm.lightingVolumeTestDSS.Get(), 1);

                    // RS
                    context.Get()->RSSetState(sm.cullfrontRS.Get());

                    // Light Volum
                    context.Get()->IASetInputLayout(sm.inputLayout_Position.Get());
                    context.Get()->VSSetShader(sm.VS_LightVolume.Get(), nullptr, 0);
                    coneVolume->UpdateWolrd(*light);
                    coneVolume->Draw(context, *camera);
                }
            }
        }
    }

    // clear
    context->OMSetDepthStencilState(nullptr, 0);
    context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    context->RSSetState(nullptr);

    // RTV - SRV hazard 방지
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    context->PSSetShaderResources(5, 1, nullSRV);  // shadowMap
    context->PSSetShaderResources(6, 1, nullSRV);  // albedo
    context->PSSetShaderResources(7, 1, nullSRV);  // normal
    context->PSSetShaderResources(8, 1, nullSRV);  // metal rough
    context->PSSetShaderResources(9, 1, nullSRV);  // emissive
    context->PSSetShaderResources(10, 1, nullSRV); // depth
    context->PSSetShaderResources(11, 1, nullSRV); // ibl - irradiance
    context->PSSetShaderResources(12, 1, nullSRV); // ibl - specular
    context->PSSetShaderResources(13, 1, nullSRV); // ibl - brdf lut
}