#include "../GlobalsPS.hlsli"

cbuffer cbFilter : register(b5)
{
    float2 g_SourceSize;
    float g_DepthThreshold;
}

Texture2D<float4> g_SourceTexture : register(t0);

float4 main(in float4 svPos : SV_POSITION, in float2 svPosition : TEXCOORD0, in float2 texCoord : TEXCOORD1) : SV_TARGET
{
    float depth = g_DepthTexture.SampleLevel(g_PointClampSampler, texCoord, 0).x;

    float4 result = 0;
    int count = 0;

    [unroll] for (int i = -3; i < 3; i++)
    {
        [unroll] for (int j = -3; j < 3; j++)
        {
            float2 cmpTexCoord = texCoord + float2(i, j) * g_SourceSize;
            float cmpDepth = g_DepthTexture.SampleLevel(g_PointClampSampler, cmpTexCoord, 0);
            float4 cmpColor = g_SourceTexture.SampleLevel(g_PointClampSampler, cmpTexCoord, 0);
            bool valid = depth - cmpDepth < g_DepthThreshold;

            result += cmpColor * valid;
            count += valid;
        }
    }

    return result / max(1, count);
}