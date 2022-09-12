#include "../GlobalsPS.hlsli"
#include "../IBLProbe.hlsli"
#include "../LightScattering.hlsli"
#include "../Shared.hlsli"
#include "../SharedPS.hlsli"

cbuffer cbFilter : register(b5)
{
    bool g_EnableSSAO;
    bool g_EnableRLR;
    float g_RLRLodParam;
}

float4 main(in float4 unused : SV_POSITION, in float4 svPos : TEXCOORD0, in float4 texCoord : TEXCOORD1) : SV_TARGET
{
    ShaderParams params = LoadParams(texCoord.xy);

    if (g_EnableSSAO)
        params.AmbientOcclusion *= g_SSAOTexture.SampleLevel(g_PointClampSampler, texCoord.xy, 0);

    float3 viewPosition = GetPositionFromDepth(svPos.xy, g_DepthTexture.SampleLevel(g_PointClampSampler, texCoord.xy, 0), g_MtxInvProjection);
    float3 position = mul(float4(viewPosition, 1.0), g_MtxInvView).xyz;
    ComputeShadingParams(params, position);

    float3 color = params.Emission;

    if (params.DeferredFlags & DEFERRED_FLAGS_IBL)
    {
        float4 ibl = 0;

        if (g_EnableRLR)
            ibl = g_RLRTexture.SampleLevel(g_LinearClampSampler, texCoord.xy, params.Roughness * g_RLRLodParam);

        if (ibl.a < 0.99)
            ibl.rgb += ComputeIndirectIBLProbes(params, position) * (1 - ibl.a);

        if (params.DeferredFlags & DEFERRED_FLAGS_GI)
        {
            float sggiBlendFactor = saturate(g_SGGIParam.x + params.Roughness * g_SGGIParam.y);
            float iblBlendFactor = lerp(1.0 - sggiBlendFactor, 1.0, params.Metalness);
            ibl.rgb *= iblBlendFactor;
        }

        params.IndirectSpecular = ibl.rgb;

        color += ComputeIndirectLighting(params, g_EnvBRDFTexture, g_LinearClampSampler);
    }

    if (params.DeferredFlags & DEFERRED_FLAGS_LIGHT_SCATTERING)
    {
        float2 lightScattering = ComputeLightScattering(position, viewPosition);
        color = color * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
    }

    return float4(color, 1.0);
}