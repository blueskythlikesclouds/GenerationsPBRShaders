#include "../../global.psparam.hlsl"
#include "../Functions.hlsl"

float4 g_SourceSize_DepthThreshold : register(c150);
sampler2D g_SourceSampler : register(s4);

float4 main(in float2 texCoord : TEXCOORD0) : COLOR
{
    float depth = LinearizeDepth(tex2Dlod(g_DepthSampler, float4(texCoord.xy, 0, 0)).x, g_MtxInvProjection);

    float4 result = 0;
    int count = 0;

    for (int i = -4; i < 4; i++)
    {
        for (int j = -4; j < 4; j++)
        {
            float4 cmpTexCoord = float4(texCoord + float2(i, j) * g_SourceSize_DepthThreshold.xy, 0, 0);
            float cmpDepth = LinearizeDepth(tex2Dlod(g_DepthSampler, cmpTexCoord).x, g_MtxInvProjection);

            if (depth - cmpDepth > g_SourceSize_DepthThreshold.z)
                continue;

            result += tex2Dlod(g_SourceSampler, cmpTexCoord);
            count++;
        }
    }

    return result / max(1, count);
}