#ifndef SHARED_PS_HLSLI_INCLUDED
#define SHARED_PS_HLSLI_INCLUDED

#include "Shared.hlsli"
#include "GlobalsPS.hlsli"

Texture2D<float4> g_GBuffer0 : register(t0);
Texture2D<float4> g_GBuffer1 : register(t1);
Texture2D<float4> g_GBuffer2 : register(t2);
Texture2D<float4> g_GBuffer3 : register(t3);

void ComputeShadingParams(inout ShaderParams params, float3 position)
{
    params.ViewDirection = normalize(g_EyePosition.xyz - position.xyz);
    params.CosViewDirection = saturate(dot(params.ViewDirection, params.Normal));

    params.RoughReflectionDirection = ComputeRoughReflectionDirection(params.Roughness, params.Normal, params.ViewDirection);
    params.SmoothReflectionDirection = 2 * params.CosViewDirection * params.Normal - params.ViewDirection;

    params.FresnelReflectance = lerp(params.Reflectance, params.Albedo, params.Metalness);
}

ShaderParams LoadParams(float2 texCoord, bool loadCdr = false)
{
    float4 gBuffer0 = g_GBuffer0.SampleLevel(g_PointClampSampler, texCoord, 0);
    float4 gBuffer1 = g_GBuffer1.SampleLevel(g_PointClampSampler, texCoord, 0);
    float4 gBuffer2 = g_GBuffer2.SampleLevel(g_PointClampSampler, texCoord, 0);
    float4 gBuffer3 = g_GBuffer3.SampleLevel(g_PointClampSampler, texCoord, 0);

    ShaderParams params = CreateShaderParams();

    params.Albedo = gBuffer1.rgb;
    params.Shadow = gBuffer1.w;
    params.Reflectance = gBuffer2.x;
    params.Roughness = gBuffer2.y;
    params.AmbientOcclusion = gBuffer2.z;
    params.Metalness = gBuffer2.w;
    params.Normal = gBuffer3.xyz * 2.0 - 1.0;
    params.DeferredFlags = uint(gBuffer3.w * DEFERRED_FLAGS_MAX);

    if (loadCdr && (params.DeferredFlags & DEFERRED_FLAGS_CDR))
        params.Cdr = gBuffer0.rgb;
    else
        params.Emission = gBuffer0.rgb;

    return params;
}

void StoreParams(inout ShaderParams params, out float4 gBuffer0, out float4 gBuffer1, out float4 gBuffer2, out float4 gBuffer3)
{
    params.DeferredFlags = DEFERRED_FLAGS_LIGHT | DEFERRED_FLAGS_IBL | DEFERRED_FLAGS_LIGHT_SCATTERING;
#ifdef NoGI
    params.DeferredFlags |= DEFERRED_FLAGS_SH_LIGHT_FIELD;
#endif

#if defined HasSamplerCdr
    params.DeferredFlags |= DEFERRED_FLAGS_CDR;
    gBuffer0.rgb = params.Cdr;
#elif defined NoGI
    gBuffer0.rgb = params.Emission;
#else
    gBuffer0.rgb = ComputeIndirectLighting(params, g_EnvBRDFTexture, g_LinearClampSampler);
    gBuffer0.rgb += params.Emission;
#endif

    gBuffer0.w = 0.0; // Unused.

    gBuffer1.rgb = params.Albedo;
    gBuffer1.w = params.Shadow;

    gBuffer2.x = params.Reflectance;
    gBuffer2.y = params.Roughness;
    gBuffer2.z = params.AmbientOcclusion;
    gBuffer2.w = params.Metalness;

    gBuffer3.xyz = params.Normal * 0.5 + 0.5;
    gBuffer3.w = (float(params.DeferredFlags) + 0.5) / float(DEFERRED_FLAGS_MAX);
}

void StoreParams(float3 emission, float3 normal, int flags, out float4 gBuffer0, out float4 gBuffer1, out float4 gBuffer2, out float4 gBuffer3)
{
    gBuffer0.rgb = emission;
    gBuffer0.w = 0.0;

    gBuffer1 = 0.0;

    gBuffer2.x = 0.0;
    gBuffer2.y = 1.0;
    gBuffer2.z = 0.0;
    gBuffer2.w = 1.0;

    gBuffer3.xyz = normal * 0.5 + 0.5;
    gBuffer3.w = (float(flags) + 0.5) / float(DEFERRED_FLAGS_MAX);
}

float3 ComputeLocalLights(in ShaderParams params, float3 position)
{
    float3 color = 0;

    for (int i = 0; i < mrgLocalLightCount; i++)
    {
        float4 positionAndRange = mrgLocalLightData[i * 2 + 0];
        float4 colorAndInvRange = mrgLocalLightData[i * 2 + 1];

        color += ComputeLocalLight(position, params, positionAndRange.xyz, colorAndInvRange.rgb, float2(positionAndRange.w, colorAndInvRange.w));
    }

    return color;
}

float GetToneMapLuminance()
{
    return g_LuminanceTexture.Load(int3(0, 0, 0)) * g_RcpMiddleGray;
}

#endif