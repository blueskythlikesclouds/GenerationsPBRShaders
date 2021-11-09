#include "../../global.psparam.hlsl"
#include "../Functions.hlsl"

float4 g_SSAOSize_DepthThreshold : register(c150);
sampler2D g_SSAOSampler : register(s4);

float4 main(in float2 texCoord : TEXCOORD0) : COLOR
{
    float depth = LinearizeDepth(tex2Dlod(g_DepthSampler, float4(texCoord.xy, 0, 0)).x, g_MtxInvProjection);

    float result = 0;
    int count = 0;

    for (int i = -4; i < 4; i++)
    {
        for (int j = -4; j < 4; j++)
        {
            float4 cmpTexCoord = float4(texCoord + float2(i, j) * g_SSAOSize_DepthThreshold.xy, 0, 0);
            float cmpDepth = LinearizeDepth(tex2Dlod(g_DepthSampler, cmpTexCoord).x, g_MtxInvProjection);

            if (cmpDepth - depth > g_SSAOSize_DepthThreshold.z)
                continue;

            result += tex2Dlod(g_SSAOSampler, cmpTexCoord).x;
            count++;
        }
    }

    return result / max(1, count);
}