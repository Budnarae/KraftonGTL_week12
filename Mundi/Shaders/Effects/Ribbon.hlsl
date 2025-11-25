// Ribbon.hlsl - Shader for rendering particle ribbon/trail effects

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
    float UVStart;      // Segment start U (0~1)
    float UVEnd;        // Segment end U (0~1)
    float UseTexture;   // Use texture (0 or 1)
    float Alpha;        // Segment alpha for fade out
    float3 Padding;     // Padding for 16-byte alignment
}

struct VS_INPUT
{
    float3 position : POSITION;
    float2 uv       : TEXCOORD0;
    float4 color    : COLOR;
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

Texture2D RibbonTex : register(t0);
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

    if (UseTexture > 0.5f)
    {
        // Remap UV along ribbon length
        float2 remappedUV;
        remappedUV.x = lerp(UVStart, UVEnd, i.uv.x);
        remappedUV.y = i.uv.y;

        float4 texColor = RibbonTex.Sample(LinearSamp, remappedUV);
        finalColor = texColor * Color;
    }
    else
    {
        // Solid color
        finalColor = Color;
    }

    // Apply segment alpha for trail fade out
    finalColor.a *= Alpha;

    // Discard fully transparent pixels
    if (finalColor.a < 0.01f)
    {
        discard;
    }

    Output.Color = finalColor;
    Output.UUID = UUID;

    return Output;
}
