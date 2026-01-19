// [ VS_BaseLit_Model ]

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
    
    // clip space
    output.pos = mul(float4(input.pos, 1.0f), finalWorld); // local -> world
    output.worldPos = output.pos.xyz; // (world pos 저장)
    output.pos = mul(output.pos, view); // world -> view
    output.pos = mul(output.pos, projection); // view -> clip
    
    // world TBN
    float3 tangent = normalize(mul(input.tangent, (float3x3) finalWorld));
    float3 bitangent = normalize(mul(input.bitangent, (float3x3) finalWorld));
    output.normal = normalize(mul(input.normal, (float3x3) finalWorld));
    output.TBN = float3x3(tangent, bitangent, output.normal);
    
    // uv
    output.texCoord = input.texCoord;
    
    // Light Clip Space Position
    output.posShadow = mul(float4(output.worldPos, 1), shadowView);
    output.posShadow = mul(output.posShadow, shadowProjection);

    return output;
}