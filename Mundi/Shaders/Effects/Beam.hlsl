// Beam.hlsl - Shader for rendering particle beam effects

cbuffer ModelBuffer : register(b0)
{
    row_major float4x4 WorldMatrix;
    row_major float4x4 WorldInverseTranspose;
};

cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
};

cbuffer ColorId : register(b3)
{
    float4 Color;
    uint UUID;
    float UVStart;    // 세그먼트 시작 U (0~1)
    float UVEnd;      // 세그먼트 끝 U (0~1)
    float UseTexture; // 텍스처 사용 여부 (0 또는 1)
}

struct VS_INPUT
{
    float3 position : POSITION;   // Vertex position
    float2 uv       : TEXCOORD0;  // Texture coordinates
    float4 color    : COLOR;      // Vertex color (optional)
};

struct PS_INPUT
{
    float4 pos   : SV_POSITION;
    float2 uv    : TEXCOORD0;
    float4 color : COLOR;
};

struct PS_OUTPUT
{
    float4 Color : SV_Target0;
    uint UUID    : SV_Target1;
};

Texture2D BeamTex : register(t0);
SamplerState LinearSamp : register(s0);

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT o;

    // Transform to world space
    float4 worldPos = mul(float4(input.position, 1.0f), WorldMatrix);

    // Transform to clip space
    o.pos = mul(worldPos, mul(ViewMatrix, ProjectionMatrix));
    o.uv = input.uv;
    o.color = input.color;

    return o;
}

PS_OUTPUT mainPS(PS_INPUT i)
{
    PS_OUTPUT Output;

    float4 finalColor;

    // 정점 색상과 인스턴스 색상을 곱함 (리본: 정점 색상 사용, 빔: 인스턴스 색상만 사용)
    float4 tintColor = i.color * Color;

    if (UseTexture > 0.5f)
    {
        // 빔 전체 UV로 리매핑 (시작=0, 끝=1)
        float2 remappedUV;
        remappedUV.x = lerp(UVStart, UVEnd, i.uv.x);  // 세그먼트 위치에 따라 0~1
        remappedUV.y = i.uv.y;  // 너비 방향은 그대로

        float4 texColor = BeamTex.Sample(LinearSamp, remappedUV);
        finalColor = texColor * tintColor;  // 텍스처 색상 * 틴트

        // 알파 컷오프 - 어두운/투명한 픽셀 제거
        // 밝기(luminance) 또는 알파가 임계값 미만이면 discard
        float luminance = dot(texColor.rgb, float3(0.299f, 0.587f, 0.114f));
        float alphaThreshold = 0.1f;  // 조절 가능한 임계값

        if (texColor.a < alphaThreshold || luminance < alphaThreshold)
        {
            discard;
        }
    }
    else
    {
        // 텍스처 없이 단색
        finalColor = tintColor;
    }

    Output.Color = finalColor;
    Output.UUID = UUID;

    return Output;
}
