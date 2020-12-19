#include "../../global.psparam.hlsl"
#include "../Functions.hlsl"

float4 g_Param : register(c150);

sampler2D s0 : register(s0);

float4 main(in float4 texCoord : TEXCOORD) : COLOR
{
    float v0 = tex2Dlod(s0, float4(texCoord.xy,                                0, 0)).x;
    float v1 = tex2Dlod(s0, float4(texCoord.xy + float2(g_Param.x, 0),         0, 0)).x;
    float v2 = tex2Dlod(s0, float4(texCoord.xy + float2(0,         g_Param.y), 0, 0)).x;
    float v3 = tex2Dlod(s0, float4(texCoord.xy + float2(g_Param.x, g_Param.y), 0, 0)).x;

    float vMin = min(min(v0, v1), min(v2, v3));
    float vMax = max(max(v0, v1), max(v2, v3));

    return float4(LinearizeDepth(vMin, g_MtxInvProjection), 0, 0, 0);
}