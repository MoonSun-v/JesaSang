#include "PostProcessPass.h"
#include "../../Manager/ShaderManager.h"
#include "../../Manager/WorldManager.h"

// [ PostProcess Pass ]
// ToneMapping(LDR) + PostProcess
// Tone Mapping 패스는 화면을 덮는 FullScreen 사각형을 그리면서,
// HDR SRV를 샘플링해 색을 계산하고, 그 결과를 BackBuffer에 기록하는 단계
void PostProcessPass::Execute(ComPtr<ID3D11DeviceContext>& context, RenderQueue& queue, Camera* cam)
{
    auto& sm = ShaderManager::Instance();

    // clear
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    context->PSSetShaderResources(14, 1, nullSRV);
    context->PSSetShaderResources(16, 1, nullSRV);

    ID3D11RenderTargetView* nullRTVs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, nullRTVs, nullptr);

    // RTV
    context->RSSetViewports(1, &sm.viewport_screen);
    context->OMSetRenderTargets(1, sm.backBufferRTV.GetAddressOf(), nullptr);
    context->ClearRenderTargetView(sm.backBufferRTV.Get(), clearColor);

    // IA
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(nullptr);

    // Shader
    context->VSSetShader(sm.VS_FullScreen.Get(), NULL, 0);
    context->PSSetShader(sm.PS_PostProcess.Get(), NULL, 0);

    // Sampler
    context->PSSetSamplers(0, 1, sm.linearSamplerState.GetAddressOf());

    // SRV
    context->PSSetShaderResources(14, 1, sm.sceneHDRSRV.GetAddressOf());
    context->PSSetShaderResources(16, 1, sm.finalBloomSRV.GetAddressOf());

    // CB
    PostProcessWorldData& data = WorldManager::Instance().postProcessData;

    sm.postProcessCBData.isHDR = data.isHDR;
    sm.postProcessCBData.useDefaultGamma = data.useDefaultGamma;
    sm.postProcessCBData.defaultGamma = data.defaultGamma;
    sm.postProcessCBData.exposure = data.exposure;

    sm.postProcessCBData.useColorAdjustments = data.useColorAdjustments;
    sm.postProcessCBData.useWhiteBalance = data.useWhiteBalance;
    sm.postProcessCBData.useLGG = data.useLGG;
    sm.postProcessCBData.useVignette = data.useVignette;
    sm.postProcessCBData.useFilmGrain = data.useFilmGrain;
    sm.postProcessCBData.useBloom = data.useBloom;

    sm.postProcessCBData.contrast = data.contrast;
    sm.postProcessCBData.saturation = data.saturation;
    sm.postProcessCBData.useHueShift = data.useHueShift;
    sm.postProcessCBData.hueShift = data.hueShift;
    sm.postProcessCBData.useColorTint = data.useColorTint;
    sm.postProcessCBData.colorTint = data.colorTint;
    sm.postProcessCBData.colorTint_strength = data.colorTint_strength;

    sm.postProcessCBData.temperature = data.temperature;
    sm.postProcessCBData.tint = data.tint;

    sm.postProcessCBData.useLift = data.useLift;
    sm.postProcessCBData.useGamma = data.useGamma;
    sm.postProcessCBData.useGain = data.useGain;

    sm.postProcessCBData.lift = data.lift;
    sm.postProcessCBData.lift_strength = data.lift_strength;
    sm.postProcessCBData.gamma = data.gamma;
    sm.postProcessCBData.gamma_strength = data.gamma_strength;
    sm.postProcessCBData.gain = data.gain;
    sm.postProcessCBData.gain_strength = data.gain_strength;

    sm.postProcessCBData.vignette_intensity = data.vignette_intensity;
    sm.postProcessCBData.vignette_smoothness = data.vignette_smoothness;
    sm.postProcessCBData.vignetteCenter = data.vignetteCenter;
    sm.postProcessCBData.vignetteColor = data.vignetteColor;

    sm.postProcessCBData.grain_intensity = data.grain_intensity;
    sm.postProcessCBData.grain_response = data.grain_response;
    sm.postProcessCBData.grain_scale = data.grain_scale;

    sm.bloomCBData.bloom_threshold = data.bloom_threshold;
    sm.bloomCBData.bloom_intensity = data.bloom_intensity;
    sm.bloomCBData.bloom_scatter = data.bloom_scatter;
    sm.bloomCBData.bloom_clamp = data.bloom_clamp;
    sm.bloomCBData.bloom_tint = data.bloom_tint;
    sm.bloomCBData.srcMip = data.srcMip;
    sm.bloomCBData.srcTexelSize = data.srcTexelSize;
    
    context->UpdateSubresource(sm.postProcessCB.Get(), 0, nullptr, &sm.postProcessCBData, 0, 0);
    context->UpdateSubresource(sm.bloomCB.Get(), 0, nullptr, &sm.bloomCBData, 0, 0);


    // Draw Call
    context.Get()->Draw(3, 0);

    // cleanup
    context->PSSetShaderResources(14, 1, nullSRV);
}