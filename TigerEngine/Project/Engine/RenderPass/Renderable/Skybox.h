#pragma once
#include "pch.h"

/*
    [ Skybox ]

    SkyboxPass (Renderer)가 소유하고 Draw하는 Renderable 객체입니다.

*/

class SkyBox
{
    ComPtr<ID3D11Buffer> vertexBuffer = nullptr;
    ComPtr<ID3D11Buffer> indexBuffer = nullptr;
    ComPtr<ID3D11ShaderResourceView> skyboxSRV = nullptr;

    UINT vertexBufferStride = 0;
    UINT vertexBufferOffset = 0;
    UINT indexCount = 0;

public:
    void Create(ComPtr<ID3D11Device>& device, const std::wstring& filePath);
    void Draw(ComPtr<ID3D11DeviceContext>& context, const Matrix& view, const Matrix& projection) const;
};

