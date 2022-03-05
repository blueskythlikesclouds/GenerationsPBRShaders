#include "../GlobalsPS.hlsli"
#include "../Shared.hlsli"
#include "../SharedPS.hlsli"
#include "../SHLightField.hlsli"

cbuffer cbFilter : register(b5)
{
    bool g_EnableSSAO;
}

float4 main(in float4 unused : SV_POSITION, in float4 svPos : TEXCOORD0, in float4 texCoord : TEXCOORD1) : SV_TARGET
{
    ShaderParams params = LoadParams(texCoord.xy, true);

    if (g_EnableSSAO)
        params.AmbientOcclusion *= g_SSAOTexture.SampleLevel(g_PointClampSampler, texCoord.xy, 0);

    float3 viewPosition = GetPositionFromDepth(svPos.xy, g_DepthTexture.SampleLevel(g_PointClampSampler, texCoord.xy, 0), g_MtxInvProjection);
    float3 position = mul(float4(viewPosition, 1.0), g_MtxInvView).xyz;
    ComputeShadingParams(params, position);

    float3 color = params.Emission;

    if (params.DeferredFlags & DEFERRED_FLAGS_SH_LIGHT_FIELD)
    {
        ComputeSHLightField(params, position);
        color += ComputeIndirectLighting(params, g_EnvBRDFTexture, g_PointClampSampler);
    }

    if (params.DeferredFlags & DEFERRED_FLAGS_LIGHT)
    {
        if (params.DeferredFlags & DEFERRED_FLAGS_SH_LIGHT_FIELD)
            params.Shadow *= ComputeShadow(g_ShadowMapTexture, g_PointBorderSampler, mul(float4(position, 1.0), g_MtxLightViewProjection), g_ShadowMapSize, g_ESMFactor);
        else
            params.Shadow *= ComputeShadow(g_VerticalShadowMapTexture, g_PointBorderSampler, mul(float4(position, 1.0), g_MtxLightViewProjection), g_ShadowMapSize, g_ESMFactor);

        color += ComputeDirectLighting(params, -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, params.DeferredFlags & DEFERRED_FLAGS_CDR) * params.Shadow;
        color += ComputeLocalLights(params, position);
    }

    return float4(color, 1.0);
}