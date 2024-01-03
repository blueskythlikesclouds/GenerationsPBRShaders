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

    [branch] if (g_EnableSSAO)
    {
        float ambientOcclusion = g_SSAOTexture.SampleLevel(g_PointClampSampler, texCoord.xy, 0);

        if (params.DeferredFlags & DEFERRED_FLAGS_GI)
            params.Emission *= ambientOcclusion;

        params.AmbientOcclusion *= ambientOcclusion;
    }

    float3 viewPosition = GetPositionFromDepth(svPos.xy, g_DepthTexture.SampleLevel(g_PointClampSampler, texCoord.xy, 0), g_MtxInvProjection);
    float3 position = mul(float4(viewPosition, 1.0), g_MtxInvView).xyz;
    ComputeShadingParams(params, position);

    float3 albedo = params.Albedo;

    [branch] if (params.DeferredFlags & DEFERRED_FLAGS_TRANSLUCENCY)
    {
        // Shadowing
        float3 halfwayDirection = normalize(params.ViewDirection - mrgGlobalLight_Direction.xyz);
        float3 cmpWorldPos = position + halfwayDirection * 0.04;
        float3 cmpViewPos = mul(float4(cmpWorldPos, 1), g_MtxView).xyz;
        float4 cmpProjPos = mul(float4(cmpViewPos, 1), g_MtxProjection);
        cmpProjPos.xyz /= cmpProjPos.w;
        cmpProjPos.xy = cmpProjPos.xy * float2(0.5, -0.5) + 0.5;

        float visibility = LinearizeDepth(g_DepthTexture.SampleLevel(g_PointClampSampler, cmpProjPos.xy, 0).x, g_MtxInvProjection) - cmpViewPos.z > 0.04;
        params.Albedo = lerp(params.Albedo, params.Albedo * 0.5, visibility * saturate(dot(halfwayDirection, -mrgGlobalLight_Direction.xyz)));

        // Rim light
        cmpWorldPos = position + normalize(params.ViewDirection + params.Normal) * 0.04;
        cmpViewPos = mul(float4(cmpWorldPos, 1), g_MtxView).xyz;
        cmpProjPos = mul(float4(cmpViewPos, 1), g_MtxProjection);
        cmpProjPos.xyz /= cmpProjPos.w;
        cmpProjPos.xy = cmpProjPos.xy * float2(0.5, -0.5) + 0.5;

        visibility = viewPosition.z - LinearizeDepth(g_DepthTexture.SampleLevel(g_PointClampSampler, cmpProjPos.xy, 0).x, g_MtxInvProjection) > 0.04;
        params.Albedo = lerp(params.Albedo, saturate(params.Albedo * 2.0), visibility * (1 - saturate(dot(params.ViewDirection, params.Normal))));
    }

    float3 color = params.Emission;

    [branch] if (params.DeferredFlags & DEFERRED_FLAGS_LIGHT_FIELD)
    {
        if (g_GIColorOverride.r >= 0.0 && g_GIColorOverride.g >= 0.0 && g_GIColorOverride.b >= 0.0)
            params.IndirectDiffuse = g_GIColorOverride.rgb;
        else
            ComputeSHLightField(params, position);

        color += ComputeIndirectLighting(params, g_EnvBRDFTexture, g_PointClampSampler);
    }

    [branch] if (params.DeferredFlags & DEFERRED_FLAGS_LIGHT)
    {
        [branch] if (params.DeferredFlags & DEFERRED_FLAGS_LIGHT_FIELD)
            params.Shadow = ComputeShadow(g_ShadowMapTexture, g_PointBorderSampler, g_ShadowMapSize, g_ESMFactor, mul(float4(position, 1.0), g_MtxLightViewProjection), params.Shadow);
        else
            params.Shadow *= ComputeShadow(g_VerticalShadowMapTexture, g_PointBorderSampler, g_ShadowMapSize, g_ESMFactor, mul(float4(position, 1.0), g_MtxLightViewProjection));

        color += ComputeDirectLighting(params, -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, params.DeferredFlags & DEFERRED_FLAGS_CDR) * params.Shadow;
        color += ComputeLocalLights(params, position);
    }

    [branch] if (params.DeferredFlags & DEFERRED_FLAGS_TRANSLUCENCY)
    {
        float vDotL = dot(params.ViewDirection, -mrgGlobalLight_Direction.xyz);
        float wrap = saturate((-dot(params.Normal, -mrgGlobalLight_Direction.xyz) + 0.5) / ((1 + 0.5) * (1 + 0.5)));
        float scatter = NdfGGX(saturate(-vDotL), 0.6);
        color += mrgGlobalLight_Diffuse.rgb * wrap * scatter * albedo * params.Shadow;
    }

    return float4(color, 1.0);
}