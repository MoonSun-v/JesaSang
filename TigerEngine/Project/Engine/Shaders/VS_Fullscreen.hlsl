// [ VS_Fullscreen ]
// PostProcessing 단계를 위한 VS.
// SV_VertexID 시멘틱 : 현재 Vertex Shader가 처리 중인 정점의 인덱스 번호

#include "shared.fxh"

PS_FullScreen_Input main(uint id : SV_VertexID)
{
    PS_FullScreen_Input output = (PS_FullScreen_Input) 0;
    
    // NDC 범위 -1~1을 의도적으로 초과하여
    // 래스터라이저가 Vieport로 클리핑하여 화면의 모든 픽셀을 커버함
    float2 pos[3] =
    {
        float2(-1.0f, -1.0f),
        float2(-1.0f, 3.0f),
        float2(3.0f, -1.0f)
    };
    
    float2 uv[3] =
    {
        float2(0.0f, 1.0f),
        float2(0.0f, -1.0f),
        float2(2.0f, 1.0f)
    };
    
    output.position = float4(pos[id], 0.0f, 1.0f); // 2D screen (z:0, w:1)
    output.uv = uv[id];
    return output;
}