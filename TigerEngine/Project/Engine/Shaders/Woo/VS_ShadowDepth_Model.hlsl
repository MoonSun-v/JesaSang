// [ VS_ShadowDepth_Model ]
// Mesh의 ShadowMap (Depth Texture) 생성을 위한 VertexShader

#include <shared.fxh>

PS_INPUT main(VS_Weight_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    Matrix finalWorld;
    
    // skeletal
    if (isRigid == 0)
    {
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
       
        finalWorld = mul(weightedOffsetPose, world);
        output.finalWorld = finalWorld;
    }
    // static, rigid
    else
    {
        finalWorld = mul(bonePose[refBoneIndex], world);
    }
    
    // view clip space (shadowView, shadowProjection)
    output.pos = mul(input.pos, finalWorld);
    output.worldPos = output.pos;
    output.pos = mul(output.pos, shadowView);
    output.pos = mul(output.pos, shadowProjection);

    return output;
}