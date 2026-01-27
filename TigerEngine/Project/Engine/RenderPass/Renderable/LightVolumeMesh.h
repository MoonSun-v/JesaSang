#pragma once
#define NOMINMAX

#pragma once
#include "pch.h"

class Light;
class Camera;
struct Position_Vertex;

// Light Volume Type
enum class LightVolumeType {
    Sphere,
    Cone
};

/*
    [Light Volume Mesh]

    Deferred Rendering의 멀티 라이트 처리(Point, Spot)시에
    라이팅 연산 영역을 필터링 하기 위해 사용하는 Mesh

    - Stencil Pass를 통해 라이팅 연산 후보 픽셀을 마킹하고
    - Lighting Pass에서 Stencil Test를 통해 해당 픽셀들에 대해서만 라이팅을 계산한다.
    - Lighting Volume Rendering은 카메라가 볼륨 외부에 있는 경우에만 호출된다.
      * Inside : Stencil Test off + FullScreenPass
      * Outside : Stencil Test on + CullBack + Lighting Volume Pass
*/

class LightVolumeMesh
{
private:  
    // volume type
    LightVolumeType volumeType;

    // vertex, index
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    UINT indexCount;
    UINT stride;
    DXGI_FORMAT indexFormat;

    // matrix
    Matrix world;

public:
    LightVolumeMesh();
    void UpdateWolrd(Light& light);
    void Draw(ComPtr<ID3D11DeviceContext>& context, const Camera& camera) const;

    bool IsInsidePointLight(const Vector3& camPos, const Vector3& lightPos, float radius) const;
    bool IsInsideSpotLight(const Vector3& camPos, const Vector3& lightPos,
        const Vector3& lightDirNormalized, float range, float outerAngleRadians) const;

    // friend
    friend LightVolumeMesh* CreateLightVolumeSphere(ID3D11Device* device, int slices, int stacks);
    friend LightVolumeMesh* CreateLightVolumeCone(ID3D11Device* device, int slices, bool capBase);
    friend void CreateBuffer(ID3D11Device* device, const std::vector<Position_Vertex>& verts,
        const std::vector<uint32_t>& indices, LightVolumeMesh* outMesh);
};

// 외부 Create Functions
LightVolumeMesh* CreateLightVolumeSphere(ID3D11Device* device, int slices = 24, int stacks = 16);
LightVolumeMesh* CreateLightVolumeCone(ID3D11Device* device, int slices = 24, bool capBase = false);


