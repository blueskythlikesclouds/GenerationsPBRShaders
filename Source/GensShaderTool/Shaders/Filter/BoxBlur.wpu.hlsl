#include "../../global.psparam.hlsl"
#include "../Functions.hlsl"

float4 g_SourceSize_DepthThreshold : register(c150);
sampler2D g_SourceSampler : register(s4);

float4 main(in float2 texCoord : TEXCOORD0) : COLOR
{
    float depth = tex2D(g_DepthSampler, texCoord).x;

    float4 result = 0;
    int count = 0;

    [unroll] for (int i = -3; i < 3; i++)
    {
        [unroll] for (int j = -3; j < 3; j++)
        {
            float2 cmpTexCoord = texCoord + float2(i, j) * g_SourceSize_DepthThreshold.xy;
            float cmpDepth = tex2D(g_DepthSampler, cmpTexCoord).x;
            float4 cmpColor = tex2D(g_SourceSampler, cmpTexCoord);
            bool valid = depth - cmpDepth < g_SourceSize_DepthThreshold.z;

            result += cmpColor * valid;
            count += valid;
        }
    }

    return result / max(1, count);
}