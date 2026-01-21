#pragma once
#include "pch.h"

/*
    [ Skybox ]

    SkyboxPass (Renderer)가 소유하고 Draw하는 Renderable 객체입니다.

*/

class SkyBox
{
private:
    ComPtr<ID3D11Buffer> vertexBuffer = nullptr;
    ComPtr<ID3D11Buffer> indexBuffer = nullptr;

    ComPtr<ID3D11ShaderResourceView> skyboxSRV = nullptr;           // skybox
    ComPtr<ID3D11ShaderResourceView> ibl_irradianceSRV = nullptr;   // IBL - diffuse irradiance
    ComPtr<ID3D11ShaderResourceView> ibl_specularEnvSRV = nullptr;  // IBL - prefiltered env
    ComPtr<ID3D11ShaderResourceView> ibl_brdfLutSRV = nullptr;      // IBL - brdf lut

    UINT vertexBufferStride = 0;
    UINT vertexBufferOffset = 0;
    UINT indexCount = 0;

public:
    void Create(const ComPtr<ID3D11Device>& device, const wchar_t* skyboxPath,
        const wchar_t* irradiPath, const wchar_t* envPath, const wchar_t* lutPath);
    void Draw(ComPtr<ID3D11DeviceContext>& context) const;
};

