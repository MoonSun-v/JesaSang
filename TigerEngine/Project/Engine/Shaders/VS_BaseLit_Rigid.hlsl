// [ VS_BaseLit_Rigid  ]
// Rigid Skeletal Mesh Vertex Shader

#include <shared.fxh>

PS_INPUT main(VS_Rigid_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    // wolrd
    Matrix finalWorld = mul(model, world);
    output.finalWorld = finalWorld;
    
    // clip space
    output.pos = mul(float4(input.pos, 1.0f), finalWorld); // local -> world
    output.worldPos = output.pos.xyz;            // (world pos 저장)
    output.pos = mul(output.pos, view);          // world -> view
    output.pos = mul(output.pos, projection);    // view -> clip
    
    // world TBN
    float3 tangent = normalize(mul(input.tangent, (float3x3) finalWorld));
    float3 bitangent = normalize(mul(input.bitangent, (float3x3) finalWorld));
    output.worldNormal = normalize(mul(input.normal, (float3x3) finalWorld));
    output.TBN = float3x3(tangent, bitangent, output.worldNormal);
    
    // uv
    output.texCoord = input.texcoord;
    
    // Light Clip Space Position
    output.posShadow = mul(float4(output.worldPos, 1), shadowView);
    output.posShadow = mul(output.posShadow, shadowProjection);

    return output;
}