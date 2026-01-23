// [ VS_ShadowDepth_Rigid ]
// Static / Rigid Mesh의 ShadowMap (Depth Texture) 생성을 위한 VertexShader

#include <shared.fxh>

PS_INPUT main(VS_Weight_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    // wolrd
    Matrix finalWorld = mul(model, world);
    output.finalWorld = finalWorld;
    
    // view clip space (shadowView, shadowProjection)
    output.pos = mul(float4(input.pos, 1.0f), finalWorld);
    output.worldPos = output.pos.xyz;
    output.pos = mul(output.pos, shadowView);
    output.pos = mul(output.pos, shadowProjection);
    output.texCoord = input.texCoord;

    return output;
}