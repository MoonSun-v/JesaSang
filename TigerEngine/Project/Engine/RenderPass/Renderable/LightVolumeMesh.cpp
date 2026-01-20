#include "LightVolumeMesh.h"
#include "../../Components/Light.h"
#include "../../Components/Camera.h"
#include "../../../Base/Datas/Vertex.h"
#include "../../../Base/Datas/ConstantBuffer.hpp"
#include "../../Manager/ShaderManager.h"


LightVolumeMesh::LightVolumeMesh()
    : indexCount(0), stride(sizeof(Position_Vertex)),
    indexFormat(DXGI_FORMAT_R32_UINT), world(Matrix::Identity)
{
}

void LightVolumeMesh::UpdateWolrd(const Light& light)
{
    if (volumeType == LightVolumeType::Sphere)
    {
        Matrix S = Matrix::CreateScale(light.range);
        Matrix T = Matrix::CreateTranslation(light.position);
        world = S * T;
    }
    else if (volumeType == LightVolumeType::Cone)
    {
        float height = light.range;
        float outerRad = DirectX::XMConvertToRadians(light.outerAngle);
        float radius = height * tanf(outerRad);
        
        Vector3 coneForward(0, 0, 1);
        Vector3 dir = light.direction;
        dir.Normalize();
        Vector3 axis = coneForward.Cross(dir);
        float angle = acosf(coneForward.Dot(dir));

        Matrix S = Matrix::CreateScale(radius, radius, height);
        Matrix R = axis.LengthSquared() < 0.0001f
            ? Matrix::Identity
            : Matrix::CreateFromAxisAngle(axis, angle);
        Matrix T = Matrix::CreateTranslation(light.position);

        world = S * R * T;
    }
}

void LightVolumeMesh::Draw(ComPtr<ID3D11DeviceContext>& context, const Camera& camera) const
{
    auto& sm = ShaderManager::Instance();

    // CB
    sm.transformCBData.world = XMMatrixTranspose(world);
    context->UpdateSubresource(sm.transformCB.Get(), 0, nullptr, &sm.transformCBData, 0, 0);

    // VB, IB
    UINT offset = 0;
    context.Get()->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    context.Get()->IASetIndexBuffer(indexBuffer.Get(), indexFormat, 0);

    // Draw Call
    context.Get()->DrawIndexed(indexCount, 0, 0);
}

bool LightVolumeMesh::IsInsidePointLight(const Vector3& camPos, const Vector3& lightPos, float radius) const
{
    float dx = camPos.x - lightPos.x;
    float dy = camPos.y - lightPos.y;
    float dz = camPos.z - lightPos.z;
    float dist2 = dx * dx + dy * dy + dz * dz;
    return dist2 <= radius * radius;
}

bool LightVolumeMesh::IsInsideSpotLight(const Vector3& camPos, const Vector3& lightPos,
    const Vector3& lightDirNormalized,  float range, float outerAngleRadians) const
{
    // v = P->C
    float vx = camPos.x - lightPos.x;
    float vy = camPos.y - lightPos.y;
    float vz = camPos.z - lightPos.z;

    // 1) range 체크 (유한 콘)
    float dist2 = vx * vx + vy * vy + vz * vz;
    if (dist2 > range * range) return false;

    // 2) 콘 각 체크: cos(theta) >= cos(outer)
    float dist = sqrtf(dist2);
    // dist==0이면 라이트 원점이므로 inside 취급
    if (dist < 1e-6f) return true;

    float invDist = 1.0f / dist;
    float nx = vx * invDist;
    float ny = vy * invDist;
    float nz = vz * invDist;

    float cosTheta = nx * lightDirNormalized.x + ny * lightDirNormalized.y + nz * lightDirNormalized.z;
    float cosOuter = cosf(outerAngleRadians);

    return cosTheta >= cosOuter;
}


// Helper..
static void CreateBuffer(ID3D11Device* device, const std::vector<Position_Vertex>& verts,
    const std::vector<uint32_t>& indices, LightVolumeMesh* outMesh);

static void BuildUnitSphere(int slices, int stacks,
    std::vector<Position_Vertex>& outVerts, std::vector<uint32_t>& outIndices);

static void BuildUnitCone(int slices, bool capBase,
    std::vector<Position_Vertex>& outVerts, std::vector<uint32_t>& outIndices);

// Create Method
LightVolumeMesh* CreateLightVolumeSphere(ID3D11Device* device, int slices, int stacks)
{
    std::vector<Position_Vertex> verts;
    std::vector<uint32_t> indices;
    BuildUnitSphere(slices, stacks, verts, indices);

    LightVolumeMesh* mesh = new LightVolumeMesh();
    CreateBuffer(device, verts, indices, mesh);
    mesh->volumeType = LightVolumeType::Sphere;
    return mesh;
}

LightVolumeMesh* CreateLightVolumeCone(ID3D11Device* device, int slices, bool capBase)
{
    std::vector<Position_Vertex> verts;
    std::vector<uint32_t> indices;
    BuildUnitCone(slices, capBase, verts, indices);

    LightVolumeMesh* mesh = new LightVolumeMesh();
    CreateBuffer(device, verts, indices, mesh);
    mesh->volumeType = LightVolumeType::Cone;
    return mesh;
}

// Vertex Buffer / Index Buffer
void CreateBuffer(ID3D11Device* device, const std::vector<Position_Vertex>& verts,
    const std::vector<uint32_t>& indices, LightVolumeMesh* outMesh)
{
    // VB
    outMesh->indexCount = (UINT)indices.size();

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = (UINT)(verts.size() * sizeof(Position_Vertex));
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vinit = {};
    vinit.pSysMem = verts.data();

    device->CreateBuffer(&vbd, &vinit, outMesh->vertexBuffer.GetAddressOf());

    // IB
    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = (UINT)(indices.size() * sizeof(uint32_t));
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iinit = {};
    iinit.pSysMem = indices.data();

    device->CreateBuffer(&ibd, &iinit, outMesh->indexBuffer.GetAddressOf());
}

// Sphere Build Helper
void BuildUnitSphere(int slices, int stacks,
    std::vector<Position_Vertex>& outVerts, std::vector<uint32_t>& outIndices)
{
    outVerts.clear();
    outIndices.clear();

    slices = (std::max)(3, slices);
    stacks = (std::max)(2, stacks);

    // Top
    outVerts.push_back({ {0.0f, 1.0f, 0.0f} });

    // Rings (excluding poles)
    for (int stack = 1; stack <= stacks - 1; ++stack)
    {
        float v = (float)stack / (float)stacks;        // 0..1
        float phi = v * 3.1415926535f;                 // 0..PI

        float y = std::cos(phi);
        float r = std::sin(phi);

        for (int slice = 0; slice <= slices; ++slice)
        {
            float u = (float)slice / (float)slices;
            float theta = u * 2.0f * 3.1415926535f;

            float x = r * std::cos(theta);
            float z = r * std::sin(theta);

            outVerts.push_back({ {x, y, z} });
        }
    }

    // Bottom
    outVerts.push_back({ {0.0f, -1.0f, 0.0f} });

    const uint32_t top = 0;
    const uint32_t bottom = (uint32_t)outVerts.size() - 1;
    const uint32_t ringVertCount = (uint32_t)(slices + 1);

    // Top cap
    uint32_t base = 1;
    for (int slice = 0; slice < slices; ++slice)
    {
        outIndices.push_back(top);
        outIndices.push_back(base + slice);
        outIndices.push_back(base + slice + 1);
    }

    // Middle
    for (int stack = 0; stack < stacks - 2; ++stack)
    {
        uint32_t ring0 = 1 + stack * ringVertCount;
        uint32_t ring1 = ring0 + ringVertCount;

        for (int slice = 0; slice < slices; ++slice)
        {
            outIndices.push_back(ring0 + slice);
            outIndices.push_back(ring1 + slice);
            outIndices.push_back(ring1 + slice + 1);

            outIndices.push_back(ring0 + slice);
            outIndices.push_back(ring1 + slice + 1);
            outIndices.push_back(ring0 + slice + 1);
        }
    }

    // Bottom cap
    uint32_t lastRing = 1 + (uint32_t)(stacks - 2) * ringVertCount;
    for (int slice = 0; slice < slices; ++slice)
    {
        outIndices.push_back(bottom);
        outIndices.push_back(lastRing + slice + 1);
        outIndices.push_back(lastRing + slice);
    }
}

// Cone(+z) Build Helper
void BuildUnitCone(int slices, bool capBase, 
    std::vector<Position_Vertex>& outVerts, std::vector<uint32_t>& outIndices)
{
    outVerts.clear();
    outIndices.clear();

    slices = (std::max)(3, slices);

    // Apex (0,0,0)
    const uint32_t apex = 0;
    outVerts.push_back({ {0.0f, 0.0f, 0.0f} });

    // Base ring (z=1), duplicate last vertex
    const uint32_t baseStart = (uint32_t)outVerts.size();
    for (int i = 0; i <= slices; ++i)
    {
        float u = (float)i / (float)slices;
        float theta = u * 2.0f * 3.1415926535f;

        float x = std::cos(theta);
        float y = std::sin(theta);

        outVerts.push_back({ {x, y, 1.0f} });
    }

    // Side triangles
    for (int i = 0; i < slices; ++i)
    {
        uint32_t v0 = apex;
        uint32_t v1 = baseStart + i;
        uint32_t v2 = baseStart + i + 1;

        // winding: 필요하면 v1/v2 swap
        outIndices.push_back(v0);
        outIndices.push_back(v1);
        outIndices.push_back(v2);
    }

    if (capBase)
    {
        uint32_t baseCenter = (uint32_t)outVerts.size();
        outVerts.push_back({ {0.0f, 0.0f, 1.0f} });

        for (int i = 0; i < slices; ++i)
        {
            uint32_t v1 = baseStart + i;
            uint32_t v2 = baseStart + i + 1;

            // base cap triangles (fan) - winding 주의
            outIndices.push_back(baseCenter);
            outIndices.push_back(v2);
            outIndices.push_back(v1);
        }
    }
}
