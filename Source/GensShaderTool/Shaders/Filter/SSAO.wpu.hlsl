#include "../../global.psparam.hlsl"
#include "../Functions.hlsl"

#include "../Deferred/Deferred.hlsl"

float4 g_SampleCount_InvSampleCount_Radius_DistanceFade : register(c150);
float4 g_Strength : register(c151);

float4 main(in float2 vPos : TEXCOORD0, in float2 texCoord : TEXCOORD1) : COLOR
{
    float4 gBuffer3 = tex2Dlod(g_GBuffer3Sampler, float4(texCoord, 0, 0));

    uint type = UnpackPrimitiveType(gBuffer3.w);

    if (type == PRIMITIVE_TYPE_RAW || type == PRIMITIVE_TYPE_EMISSION)
        return 1.0;

    float depth = tex2Dlod(g_DepthSampler, float4(texCoord.xy, 0, 0)).x;

    float3 position = GetPositionFromDepth(vPos, depth, g_MtxInvProjection);
    float3 normal = normalize(mul(gBuffer3.xyz * 2 - 1, g_MtxView));

    float radius = g_SampleCount_InvSampleCount_Radius_DistanceFade.z / max(0.0001, -position.z);
    float noise = InterleavedGradientNoise(texCoord.xy * g_ViewportSize.xy);

    float occlusion = 0;

    for (int i = 0; i < int(g_SampleCount_InvSampleCount_Radius_DistanceFade.x); i++)
    {
        float2 cmpTexCoord = texCoord + (CalculateVogelDiskSample(i,
            g_SampleCount_InvSampleCount_Radius_DistanceFade.x, noise * 2 * PI) * 2 - 1) * radius;

        float cmpDepth = tex2Dlod(g_DepthSampler, float4(cmpTexCoord, 0, 0)).x;

        float2 cmpVPos = (cmpTexCoord - 0.5) * float2(2, -2);
        float3 cmpPos = GetPositionFromDepth(cmpVPos, cmpDepth, g_MtxInvProjection);

        float3 direction = cmpPos - position;
        float distance = length(direction);
        float invDistance = distance > 0.0001 ? 1.0f / distance : 0;

        occlusion += saturate(dot(direction * invDistance, normal)) * smoothstep(0, 1, saturate(g_SampleCount_InvSampleCount_Radius_DistanceFade.w * invDistance));
    }

    return saturate(1 - occlusion * g_SampleCount_InvSampleCount_Radius_DistanceFade.y * g_Strength.x);
}