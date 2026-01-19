// [ VS_Effect ]

#include <shared.fxh>

float2 Rotate2D(float2 v, float rad)
{
    float s = sin(rad);
    float c = cos(rad);
    return float2(v.x * c - v.y * s, v.x * s + v.y * c);
}

PS_Particle_INPUT main(VS_Particle_INPUT input)
{
    PS_Particle_INPUT output = (PS_Particle_INPUT) 0;
    
    // Frame
    uint f = (uint) input.frame;
    
    uint cols = (uint) max(1.0, atlasGrid.x);
    uint rows = (uint) max(1.0, atlasGrid.y);

    uint fx = f % cols;
    uint fy = f / cols;
    fy = min(fy, rows - 1);

    // Flipbook UV
    float2 uv = input.uv * invAtlasGrid + float2(fx, fy) * invAtlasGrid;

    // Billboard
    float3 camUp;
    float3 camRight = float3(view._11, view._21, view._31);
    
    if (billboardType == 0)       // ScreenFacing
    {
        camUp = float3(view._12, view._22, view._32);
    }
    else if (billboardType == 1)   // Y-Axis
    {
        camUp = float3(0, 1, 0);
    }

    // locar -> world -> clip
    float2 local = input.corner * (input.size * baseSizeScale);
    local = Rotate2D(local, input.rotation);

    float3 worldPos = input.pos + camRight * local.x + camUp * local.y;
    float4 viewPos = mul(float4(worldPos, 1.0f), view);
    float4 clipPos = mul(viewPos, projection);

    output.pos = clipPos;
    output.uv = uv;
    output.color = input.color;
    
    return output;
}