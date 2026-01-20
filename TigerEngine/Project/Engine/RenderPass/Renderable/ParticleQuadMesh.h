#pragma once
#include "pch.h"
#include "../../../Base/Datas/Vertex.h"

/*
    [ Particle Quad Mesh ]

    이펙트(Flipbook/Particle) 전용 Quad입니다.
    - 정점에 월드 position이 없고 corner(+uv)만 가지므로,
      일반 Mesh 파이프라인(월드행렬로 변환)에는 그대로 사용할 수 없습니다.
    - 월드 위치는 VS에서 (ParticlePos + CamRight/Up * corner * size)로 계산합니다.
    - Particle Instance Data를 받아 두 Vertexbuffer를 합쳐 바인딩합니다.
*/

class ParticleQuadMesh
{
private:
    // VB, IB
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    UINT indexCount = 6;
    UINT stride = sizeof(ParticleQuadVertex);

public:
    void Create(ComPtr<ID3D11Device>& device);
    void DrawIndexedInstanced(ComPtr<ID3D11DeviceContext>& context,
        UINT instanceCount, ID3D11Buffer* instanceVB, UINT instanceStride);
};

