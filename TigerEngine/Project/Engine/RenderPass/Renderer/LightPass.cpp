#include "LightPass.h"
#include "../Renderable/LightvolumeMesh.h"
#include "../../EngineSystem/LightSystem.h"
#include "../../EngineSystem/CameraSystem.h"
#include "../../Manager/ShaderManager.h"
#include "../../Manager/WorldManager.h"
#include "../../Object/GameObject.h"

using namespace std;

// 셰이더(PS_DeferredLighting)는 inner/outer 중 큰 각도까지 빛을 계산하므로 볼륨도 같은 각도를 써야 함
static float SpotAngleDeg(const Light& light) { return (std::max)(light.innerAngle, light.outerAngle); }

// 45도 초과 콘은 반지름(range * tan)이 range보다 커져 구보다 비효율적 -> 콘을 포함하는 구 볼륨 사용
static bool UseCone(const Light& light) { return light.type == LightType::Spot && SpotAngleDeg(light) <= 45.0f; }

LightPass::~LightPass()
{
    if (sphereVolume) delete sphereVolume;
    if (coneVolume) delete coneVolume;
}

void LightPass::Init(const ComPtr<ID3D11Device>& device)
{
    // light volume create
    sphereVolume = CreateLightVolumeSphere(device.Get(), 24, 16);
    coneVolume = CreateLightVolumeCone(device.Get(), 24, true);     // 닫힌 볼륨이어야 밑면 방향에서도 마크됨
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
        if (light->type == LightType::Directional) continue;

        LightVolumeMesh* volume = UseCone(*light) ? coneVolume : sphereVolume;
        volume->UpdateWolrd(*light);
        volume->Draw(context, *camera);
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
    Vector3 camPos = camera->GetOwner()->GetTransform()->GetWorldPosition();

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

    auto drawFullScreen = [&]()
    {
        context->OMSetDepthStencilState(sm.disableDSS.Get(), 0);
        context->RSSetState(nullptr);
        context->IASetInputLayout(nullptr);
        context->VSSetShader(sm.VS_FullScreen.Get(), NULL, 0);
        context->Draw(3, 0);
    };

    // Render
    // Emissive는 라이트 수와 무관하게 화면 전체에 한 번만 더한다 (첫 Full Screen 드로우에서)
    bool emissiveAdded = false;
    const UINT stencilRef = 0x02;       // StencilPass와 같은 값이어야 함 (ReadMask 0x02와 비교)
    for (Light*& light : lights)
    {
       auto lightPos = light->GetOwner()->GetTransform()->GetWorldPosition();

       // Directional, 카메라가 볼륨 안 : Full Screen (Stencil off)
       // Point, Spot : Light Volume + Stencil Test (StencilPass에서 마크한 영역만)
       bool useCone = UseCone(*light);
       Vector3 spotDir = light->direction;
       spotDir.Normalize();

       bool fullScreen = light->type == LightType::Directional
           || (!useCone && sphereVolume->IsInsidePointLight(camPos, lightPos, light->range))
           || (useCone && coneVolume->IsInsideSpotLight(camPos, lightPos, spotDir,
                   light->range, XMConvertToRadians(SpotAngleDeg(*light))));

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
       sm.lightingCBData.addEmissive = fullScreen && !emissiveAdded;
       emissiveAdded |= fullScreen;
       context->UpdateSubresource(sm.lightingCB.Get(), 0, nullptr, &sm.lightingCBData, 0, 0);

       if (fullScreen)
       {
           drawFullScreen();
       }
       else
       {
           context->OMSetDepthStencilState(sm.lightingVolumeTestDSS.Get(), stencilRef);
           context->RSSetState(sm.cullfrontRS.Get());
           context->IASetInputLayout(sm.inputLayout_Position.Get());
           context->VSSetShader(sm.VS_LightVolume.Get(), nullptr, 0);

           LightVolumeMesh* volume = useCone ? coneVolume : sphereVolume;
           volume->UpdateWolrd(*light);
           volume->Draw(context, *camera);
       }
    }

    // Full Screen 라이트가 하나도 없었으면 Emissive만 따로 한 번 더한다 (직접광 0, IBL 없음)
    if (!emissiveAdded)
    {
        sm.lightingCBData.lightType = static_cast<int>(LightType::Directional);
        sm.lightingCBData.isSunLight = FALSE;
        sm.lightingCBData.directIntensity = 0.0f;
        sm.lightingCBData.addEmissive = TRUE;
        context->UpdateSubresource(sm.lightingCB.Get(), 0, nullptr, &sm.lightingCBData, 0, 0);
        drawFullScreen();
    }
    sm.lightingCBData.addEmissive = FALSE;

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