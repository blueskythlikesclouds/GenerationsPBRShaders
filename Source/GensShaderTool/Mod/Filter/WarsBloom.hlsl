#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 g_BloomStar_Param1 : packoffset(c151);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> texDif0 : register(t0);
SamplerState sampDif0 : register(s0);

float4 main(in float4 unused : SV_POSITION, in float4 texCoord : TEXCOORD0) : SV_TARGET
{
    float3 color = clamp(texDif0.Sample(sampDif0, texCoord.xy).rgb, 0, 65504);
    float maxCoeff = max(0.001, max(color.r, max(color.g, color.b)));
    float factor = saturate((maxCoeff - g_BloomStar_Param1.x) / (g_BloomStar_Param1.y - g_BloomStar_Param1.x));
    return float4(color / maxCoeff * factor, 1);
}