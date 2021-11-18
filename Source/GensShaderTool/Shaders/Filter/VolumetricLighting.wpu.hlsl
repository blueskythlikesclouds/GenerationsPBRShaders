#include "../../global.psparam.hlsl"
#include "../Functions.hlsl"
#include "../Param.hlsl"

float4 g_SampleCount_InvSampleCount_G_InScatteringScale : register(c150);
float4 g_SkyDepth : register(c151);
sampler2D g_BlueNoiseSampler : register(s0);

#define G g_SampleCount_InvSampleCount_G_InScatteringScale.z

float ComputeMieScattering(float3 position)
{
    float cosTheta = dot(-mrgGlobalLight_Direction.xyz, normalize(g_EyePosition.xyz - position));
    return (1.0 - G * G) / (4.0 * PI * pow(1.0 + G * G + -2.0 * G * cosTheta, 1.5));
}

float4 main(float2 vPos : TEXCOORD0, float2 texCoord : TEXCOORD1) : COLOR
{
    float depth = tex2Dlod(g_DepthSampler, float4(texCoord, 0, 0)).x;

#if defined(IgnoreSky) && IgnoreSky
    [branch] if (depth >= 1.0)
        return 0;
#endif

    float3 viewPosition = GetPositionFromDepth(vPos, depth, g_MtxInvProjection);

#if !defined(IgnoreSky) || !IgnoreSky
    if (depth >= 1.0)
        viewPosition.z = g_SkyDepth.x;
#endif

    float3 position = mul(float4(viewPosition, 1), g_MtxInvView).xyz;

    float mieScattering = ComputeMieScattering(position);

    [branch] if (mieScattering <= 0)
        return 0;

    float noise = tex2Dlod(g_BlueNoiseSampler, float4(texCoord.xy * (g_ViewportSize.xy / 64.0), 0, 0)).x;
    float visibility = 0;

    for (int i = 0; i < int(g_SampleCount_InvSampleCount_G_InScatteringScale.x); i++)
    {
        float3 currentPosition = lerp(g_EyePosition.xyz, position, (i + noise) * g_SampleCount_InvSampleCount_G_InScatteringScale.y);

        float4 shadowMapCoords = mul(float4(currentPosition, 1), g_MtxLightViewProjection);
        if (max(abs(shadowMapCoords.x), max(abs(shadowMapCoords.y), abs(shadowMapCoords.z))) > shadowMapCoords.w)
        {

#if !defined(IgnoreSky) || !IgnoreSky
            visibility += depth >= 1.0 ? g_SampleCount_InvSampleCount_G_InScatteringScale.y : 0.0;
#endif
            continue;
        }

        shadowMapCoords.xyz /= shadowMapCoords.w;
        shadowMapCoords.xy = shadowMapCoords.xy * float2(0.5, -0.5) + 0.5;

        float shadowDepth = tex2Dlod(g_ShadowMapSampler, float4(shadowMapCoords.xy, 0, 0)).x;
        visibility += shadowDepth > shadowMapCoords.z ? g_SampleCount_InvSampleCount_G_InScatteringScale.y : 0.0;
    }

    return float4(mrgGlobalLight_Diffuse.rgb * mieScattering * visibility * g_SampleCount_InvSampleCount_G_InScatteringScale.w, 1);
}