// [ Decal Volume Vertex Shader ]
// 

#include <shared.fxh>

PS_Position_INPUT main(VS_Position_INPUT input)
{
    PS_Position_INPUT output = (PS_Position_INPUT) 0;
    
    output.pos = mul(float4(input.position, 1.0f), world);
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, projection);
    
    return output;
}