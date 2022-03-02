#include "../GlobalsPS.hlsli"
#include "../Shared.hlsli"

Texture2D<float4> g_SourceTexture : register(t0);
Texture2D<float3> g_LUTTexture : register(t1);

SamplerState g_SourceSampler : register(s0);
SamplerState g_LUTSampler : register(s1);

float4 main(in float4 svPosition : SV_POSITION, in float2 texCoord : TEXCOORD) : SV_TARGET
{
    const float2 g_LUTParam = float2(256, 16);

    float4 color = LinearToSrgb(g_SourceTexture.Sample(g_SourceSampler, texCoord));

    if (g_Debug.IsEnableLUT)
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

        float3 gradedColorL = g_LUTTexture.Sample(g_LUTSampler, lutTexCoordL);
        float3 gradedColorH = g_LUTTexture.Sample(g_LUTSampler, lutTexCoordH);

        color.rgb = lerp(gradedColorL, gradedColorH, frac(cell));
    }

    return color;
}