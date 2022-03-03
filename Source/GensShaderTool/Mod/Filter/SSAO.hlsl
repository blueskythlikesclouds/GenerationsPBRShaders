#include "../GlobalsPS.hlsli"
#include "../Shared.hlsli"
#include "../SharedPS.hlsli"

cbuffer cbFilter : register(b5)
{
    int g_SampleCount;
    float g_RcpSampleCount;
    float g_Radius;
    float g_DistanceFade;
    float g_Strength;
}

float4 main(in float4 svPos : SV_POSITION, in float2 svPosition : TEXCOORD0, in float2 texCoord : TEXCOORD1) : SV_TARGET
{
    ShaderParams params = LoadParams(texCoord);
    if (!(params.DeferredFlags & DEFERRED_FLAGS_LIGHT))
        return 1.0;

    float3 position = GetPositionFromDepth(svPos, g_DepthTexture.SampleLevel(g_PointClampSampler, texCoord, 0), g_MtxInvProjection);

    float radius = g_Radius / max(0.0001, -position.z);
    float noise = g_BlueNoiseTexture.SampleLevel(g_PointRepeatSampler, texCoord.xy * 64, 0);

    float occlusion = 0;

    for (int i = 0; i < g_SampleCount; i++)
    {
        float2 cmpTexCoord = texCoord + CalculateVogelDiskSample(i,
            g_SampleCount, noise * 2 * PI) * radius;

        float cmpDepth = g_DepthTexture.SampleLevel(g_PointClampSampler, cmpTexCoord, 0);

        float2 cmpVPos = (cmpTexCoord - 0.5) * float2(2, -2);
        float3 cmpPos = GetPositionFromDepth(cmpVPos, cmpDepth, g_MtxInvProjection);

        float3 direction = cmpPos - position;
        float distance = length(direction);
        float invDistance = distance > 0.0001 ? 1.0f / distance : 0;

        occlusion += saturate(dot(direction * invDistance, params.Normal)) * smoothstep(0, 1, saturate(g_DistanceFade * invDistance));
    }

    return saturate(1 - occlusion * g_RcpSampleCount * g_Strength);
}