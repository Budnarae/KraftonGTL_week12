#ifndef SKINNING_HLSL
#define SKINNING_HLSL

StructuredBuffer<float4x4> BoneMatrices : register(t12);
StructuredBuffer<float4x4> BoneNormalMatrices : register(t13);

float3 SkinPosition(float3 Position, uint4 BoneIndices, float4 BoneWeights)
{
    float3 Result = float3(0, 0, 0);
    
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        if (BoneWeights[i] > 0.0f)
        {
            Result += mul(float4(Position, 1.0f), transpose(BoneMatrices[BoneIndices[i]])).xyz * BoneWeights[i];;
        }
    }
    
    return Result;
}

float3 SkinNormal(float3 Normal, uint4 BoneIndices, float4 BoneWeights)
{
    float3 Result = float3(0, 0, 0);
    
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        if (BoneWeights[i] > 0.0f)
        {
            Result += mul(float4(Normal, 0.0f), transpose(BoneNormalMatrices[BoneIndices[i]])).xyz * BoneWeights[i];
        }
    }
    
    return normalize(Result);
}

float4 SkinTangent(float4 Tangent, uint4 BoneIndices, float4 BoneWeights)
{
    float3 TangentDir = float3(0, 0, 0);
    
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        if (BoneWeights[i] > 0.0f)
        {
            TangentDir += mul(float4(Tangent.xyz, 0.0f), transpose(BoneMatrices[BoneIndices[i]])).xyz * BoneWeights[i];
        }
    }
    
    return float4(normalize(TangentDir), Tangent.w); // w는 BiTangent 방향 보존
}

#endif
