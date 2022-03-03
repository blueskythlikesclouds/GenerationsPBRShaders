#include "../GlobalsPS.hlsli"
#include "../Shared.hlsli"
#include "../SharedPS.hlsli"
#include "../SHLightField.hlsli"

cbuffer cbFilter : register(b5)
{
    bool g_EnableSSAO;
}

float4 main(in float4 svPos : SV_POSITION, in float2 svPosition : TEXCOORD0, in float2 texCoord : TEXCOORD1) : SV_TARGET
{
    ShaderParams params = LoadParams(texCoord);

    if (g_EnableSSAO)
        params.AmbientOcclusion *= g_SSAOTexture.SampleLevel(g_PointClampSampler, texCoord, 0);

    float3 position = GetPositionFromDepth(svPos, g_DepthTexture.SampleLevel(g_PointClampSampler, texCoord, 0), g_MtxInvProjection);
    float3 color = params.Emission;

    if (params.DeferredFlags & DEFERRED_FLAGS_SH_LIGHT_FIELD)
    {
        ComputeSHLightField(params, position);
        color += ComputeIndirectLighting(params, g_EnvBRDFTexture, g_PointClampSampler);
    }

    if (params.DeferredFlags & DEFERRED_FLAGS_LIGHT)
    {
        color += ComputeDirectLighting(params, -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, params.DeferredFlags & DEFERRED_FLAGS_CDR);
        color += ComputeLocalLights(params, position);
    }

    return float4(color, 1.0);
}