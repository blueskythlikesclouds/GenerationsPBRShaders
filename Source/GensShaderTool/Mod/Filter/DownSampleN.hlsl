#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 g_Param : packoffset(c150);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> t0 : register(t0);
SamplerState s0 : register(s0);

float4 main(in float4 unused : SV_POSITION, in float4 texCoord : TEXCOORD0) : SV_TARGET
{
    float4 sum = 0;
    int count = 0;

    for (float x = -g_Param.x; x <= g_Param.x; x += g_Param.y)
    {
        for (float y = -g_Param.z; y <= g_Param.z; y += g_Param.w)
        {
            sum += t0.Sample(s0, texCoord.xy + float2(x, y));
            ++count;
        }
    }

    return count > 0 ? sum / count : 0;
}