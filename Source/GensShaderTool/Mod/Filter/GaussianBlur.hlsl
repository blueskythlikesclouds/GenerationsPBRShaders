#include "../GlobalsPS.hlsli"

cbuffer cbFilter : register(b5)
{
    float2 g_Offset;
    float g_Scale;
    float g_Level;
}

Texture2D<float4> g_SourceTexture : register(t4);

float4 main(in float4 unused : SV_POSITION, in float4 texCoord : TEXCOORD0) : SV_TARGET
{
    return (g_SourceTexture.SampleLevel(g_LinearClampSampler, texCoord.xy * g_Scale, g_Level) * 0.29411764705882354) +
        (g_SourceTexture.SampleLevel(g_LinearClampSampler, texCoord.xy * g_Scale + g_Offset, g_Level) * 0.35294117647058826) +
        (g_SourceTexture.SampleLevel(g_LinearClampSampler, texCoord.xy * g_Scale - g_Offset, g_Level) * 0.35294117647058826);
}