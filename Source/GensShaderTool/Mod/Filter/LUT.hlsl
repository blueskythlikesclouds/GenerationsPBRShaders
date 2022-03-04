#include "../GlobalsPS.hlsli"
#include "../Shared.hlsli"

cbuffer cbFilter : register(b5)
{
    bool g_IsEnableLUT;
}

Texture2D<float4> g_SourceTexture : register(t0);
Texture2D<float3> g_LUTTexture : register(t1);

float4 main(in float4 unused : SV_POSITION, in float4 texCoord : TEXCOORD) : SV_TARGET
{
    const float2 g_LUTParam = float2(256, 16);

    float4 color = LinearToSrgb(g_SourceTexture.SampleLevel(g_PointClampSampler, texCoord.xy, 0));

    if (g_IsEnableLUT)
    {
        float cell = color.b * (g_LUTParam.y - 1);

        float cellL = floor(cell);
        float cellH = ceil(cell);

        float floatX = 0.5 / g_LUTParam.x;
        float floatY = 0.5 / g_LUTParam.y;
        float rOffset = floatX + color.r / g_LUTParam.y * ((g_LUTParam.y - 1) / g_LUTParam.y);
        float gOffset = floatY + color.g * ((g_LUTParam.y - 1) / g_LUTParam.y);

        float2 lutTexCoordL = float2(cellL / g_LUTParam.y + rOffset, gOffset);
        float2 lutTexCoordH = float2(cellH / g_LUTParam.y + rOffset, gOffset);

        float3 gradedColorL = g_LUTTexture.SampleLevel(g_LinearClampSampler, lutTexCoordL, 0);
        float3 gradedColorH = g_LUTTexture.SampleLevel(g_LinearClampSampler, lutTexCoordH, 0);

        color.rgb = lerp(gradedColorL, gradedColorH, frac(cell));
    }

    return color;
}