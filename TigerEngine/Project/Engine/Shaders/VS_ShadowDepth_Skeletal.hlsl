// [ VS_ShadowDepth_Skeletal ]
// Skinned Skeletal Mesh의 ShadowMap (Depth Texture) 생성을 위한 VertexShader

#include <shared.fxh>

PS_INPUT main(VS_Weight_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    // skinning
    float4x4 offsetPos[4];
    offsetPos[0] = mul(boneOffset[input.boneIndices.x], bonePose[input.boneIndices.x]);
    offsetPos[1] = mul(boneOffset[input.boneIndices.y], bonePose[input.boneIndices.y]);
    offsetPos[2] = mul(boneOffset[input.boneIndices.z], bonePose[input.boneIndices.z]);
    offsetPos[3] = mul(boneOffset[input.boneIndices.w], bonePose[input.boneIndices.w]);
        
    float4x4 weightedOffsetPose;
    weightedOffsetPose = mul(input.boneWeights.x, offsetPos[0]);
    weightedOffsetPose += mul(input.boneWeights.y, offsetPos[1]);
    weightedOffsetPose += mul(input.boneWeights.z, offsetPos[2]);
    weightedOffsetPose += mul(input.boneWeights.w, offsetPos[3]);
       
    Matrix finalWorld = mul(weightedOffsetPose, world);
    output.finalWorld = finalWorld;
    
    // view clip space (shadowView, shadowProjection)
    output.pos = mul(float4(input.pos, 1.0f), finalWorld);
    output.worldPos = output.pos.xyz;
    output.pos = mul(output.pos, shadowView);
    output.pos = mul(output.pos, shadowProjection);
    output.texCoord = input.texCoord;

    return output;
}