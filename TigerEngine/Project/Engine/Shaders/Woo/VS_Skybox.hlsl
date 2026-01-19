// [ VS_Skybox ]

#include <shared.fxh>

PS_Skybox_INPUT main(VS_Position_INPUT input)
{
    PS_Skybox_INPUT output;

    output.position = mul(float4(input.position, 1.0f), view);
    output.position = mul(output.position, projection);
    
    output.position.z = output.position.w; // skybox의 depth를 최대로 고정

    // CubeMap용 방향 벡터
    output.texCoord = input.position;

    return output;
}
