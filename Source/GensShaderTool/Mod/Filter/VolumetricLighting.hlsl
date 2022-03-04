#include "../GlobalsPS.hlsli"
#include "../Shared.hlsli"

cbuffer cbFilter : register(b5)
{
    int g_SampleCount;
    float g_RcpSampleCount;
    float g_G;
    float g_InScatteringScale;
}

float ComputeMieScattering(float3 position)
{
    float cosTheta = dot(-mrgGlobalLight_Direction.xyz, normalize(g_EyePosition.xyz - position));
    return (1.0 - g_G * g_G) / (4.0 * PI * pow(1.0 + g_G * g_G + -2.0 * g_G * cosTheta, 1.5));
}


float4 main(in float4 unused : SV_POSITION, in float4 svPos : TEXCOORD0, in float4 texCoord : TEXCOORD1) : SV_TARGET
{
    float depth = g_DepthTexture.SampleLevel(g_PointClampSampler, texCoord, 0);

#ifdef IgnoreSky
    if (depth >= 1.0)
        return 0;
#endif

    float3 viewPosition = GetPositionFromDepth(svPos, depth, g_MtxInvProjection);
    float3 position = mul(float4(viewPosition, 1), g_MtxInvView).xyz;

    float mieScattering = ComputeMieScattering(position);

    if (mieScattering <= 0)
        return 0;

    float noise = g_BlueNoiseTexture.SampleLevel(g_PointRepeatSampler, texCoord.xy * (g_ViewportSize.xy / 64.0), 0);
    float visibility = 0;

    for (int i = 0; i < g_SampleCount; i++)
    {
        float3 currentPosition = lerp(g_EyePosition.xyz, position, (i + noise) * g_RcpSampleCount);

        float4 shadowMapCoords = mul(float4(currentPosition, 1), g_MtxLightViewProjection);
        if (max(abs(shadowMapCoords.x), max(abs(shadowMapCoords.y), abs(shadowMapCoords.z))) > shadowMapCoords.w)
        {
#ifndef IgnoreSky
            visibility += depth >= 1.0 ? g_RcpSampleCount : 0.0;
#endif
            continue;
        }

        shadowMapCoords.xyz /= shadowMapCoords.w;
        shadowMapCoords.xy = shadowMapCoords.xy * float2(0.5, -0.5) + 0.5;

        float shadowDepth = g_ShadowMapTexture.SampleLevel(g_PointBorderSampler, shadowMapCoords.xy, 0);
        visibility += shadowDepth > shadowMapCoords.z ? g_RcpSampleCount : 0.0;
    }

    return float4(mrgGlobalLight_Diffuse.rgb * mieScattering * visibility * g_InScatteringScale, 1);
}