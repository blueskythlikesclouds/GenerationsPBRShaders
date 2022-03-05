#include "../GlobalsPS.hlsli"
#include "../Shared.hlsli"
#include "../SharedPS.hlsli"

cbuffer cbFilter : register(b5)
{
    uint g_SampleCount;
    float g_RcpSampleCount;
    float g_Radius;
    float g_DistanceFade;
    float g_Strength;
}

float4 main(in float4 unused : SV_POSITION, in float4 svPos : TEXCOORD0, in float4 texCoord : TEXCOORD1) : SV_TARGET
{
    ShaderParams params = LoadParams(texCoord.xy);
    if (!(params.DeferredFlags & DEFERRED_FLAGS_LIGHT))
        return 1.0;

    float3 position = GetPositionFromDepth(svPos.xy, g_DepthTexture.SampleLevel(g_PointClampSampler, texCoord.xy, 0), g_MtxInvProjection);
    float3 normal = normalize(mul(float4(params.Normal, 0), g_MtxView).rgb);

    float radius = g_Radius / max(0.0001, -position.z);
    float noise = g_BlueNoiseTexture.SampleLevel(g_PointRepeatSampler, texCoord.xy * (g_ViewportSize.xy / 64.0), 0);

    float occlusion = 0;

    for (uint i = 0; i < g_SampleCount; i++)
    {
        float2 cmpTexCoord = texCoord.xy + CalculateVogelDiskSample(i,
            g_SampleCount, noise * 2 * PI) * radius;

        float cmpDepth = g_DepthTexture.SampleLevel(g_PointClampSampler, cmpTexCoord, 0);

        float2 cmpVPos = (cmpTexCoord - 0.5) * float2(2, -2);
        float3 cmpPos = GetPositionFromDepth(cmpVPos, cmpDepth, g_MtxInvProjection);

        float3 direction = cmpPos - position;
        float distance = length(direction);
        float invDistance = distance > 0.0001 ? 1.0f / distance : 0;

        occlusion += saturate(dot(direction * invDistance, normal)) * smoothstep(0, 1, saturate(g_DistanceFade * invDistance));
    }

    return saturate(1 - occlusion * g_RcpSampleCount * g_Strength);
}