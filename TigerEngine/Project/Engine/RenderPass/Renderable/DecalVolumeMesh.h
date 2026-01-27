#pragma once
#include "pch.h"

/*
    [Decal Volume Mesh]

    Decal Rendering을 위해 box volume을 그리며 
    그 영역에 decal을 그리기 위해 사용하는 Mesh
*/

class Decal;
struct Position_Vertex;

class DecalVolumeMesh
{
private:
    // vertex, index
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    UINT indexCount;
    UINT stride;
    DXGI_FORMAT indexFormat;

    // matrix
    Matrix world;

public:
    DecalVolumeMesh();
    void UpdateWolrd(Decal* decal);     // TODO :: Decal Compoent
    void Draw(ComPtr<ID3D11DeviceContext>& context, Decal* decal) const;

    // friend
    friend DecalVolumeMesh* CreateDecalVolume(ID3D11Device* device);
};

// 외부 Create Functions
DecalVolumeMesh* CreateDecalVolume(ID3D11Device* device);