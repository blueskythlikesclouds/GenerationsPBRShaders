#ifndef SHARED_PS_HLSLI_INCLUDED
#define SHARED_PS_HLSLI_INCLUDED

#include "Shared.hlsli"
#include "GlobalsPS.hlsli"

Texture2D<float4> g_GBuffer0 : register(t0);
Texture2D<float4> g_GBuffer1 : register(t0);
Texture2D<float4> g_GBuffer2 : register(t0);
Texture2D<float4> g_GBuffer3 : register(t0);

void ComputeShadingParams(inout ShaderParams params, float3 position)
{
    params.ViewDirection = normalize(g_EyePosition.xyz - position.xyz);
    params.CosViewDirection = saturate(dot(params.ViewDirection, params.Normal));

    params.RoughReflectionDirection = ComputeRoughReflectionDirection(params.Roughness, params.Normal, params.ViewDirection);
    params.SmoothReflectionDirection = 2 * params.CosViewDirection * params.Normal - params.ViewDirection;

    params.FresnelReflectance = lerp(params.Reflectance, params.Albedo, params.Metalness);
}

ShaderParams LoadParams(float2 texCoord)
{
    float4 gBuffer0 = g_GBuffer0.Sample(g_PointClampSampler, texCoord);
    float4 gBuffer1 = g_GBuffer1.Sample(g_PointClampSampler, texCoord);
    float4 gBuffer2 = g_GBuffer2.Sample(g_PointClampSampler, texCoord);
    float4 gBuffer3 = g_GBuffer3.Sample(g_PointClampSampler, texCoord);

    ShaderParams params = CreateShaderParams();

    params.Albedo = gBuffer1.rgb;
    params.Shadow = gBuffer1.w;
    params.Reflectance = gBuffer2.x;
    params.Roughness = gBuffer2.y;
    params.AmbientOcclusion = gBuffer2.z;
    params.Metalness = gBuffer2.w;
    params.Normal = gBuffer3.xyz * 2.0 - 1.0;
    params.Type = uint(gBuffer3.w * TYPE_MAX);

    return params;
}

void StoreParams(inout ShaderParams params, out float4 gBuffer0, out float4 gBuffer1, out float4 gBuffer2, out float4 gBuffer3)
{
#if defined HasSamplerCdr
    params.Type = TYPE_CDR;
    gBuffer0.rgb = params.Cdr;
#elif defined NoGI
    params.Type = TYPE_NO_GI;
    gBuffer0.rgb = params.Emission;
#else
    params.Type = TYPE_GI;
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
    gBuffer3.w = float(params.Type) / float(TYPE_MAX);
}

#endif