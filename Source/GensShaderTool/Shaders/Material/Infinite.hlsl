#ifndef SCENE_MATERIAL_INFINITE_HLSL_INCLUDED
#define SCENE_MATERIAL_INFINITE_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#include "Default.hlsl"

sampler2D diffuseSampler : register(s0);
sampler2D specularSampler : register(s1);
sampler2D normalSampler : register(s2);
sampler2D falloffSampler : register(s3);
sampler2D reflectionSampler : register(s4);

float4 FalloffFactor : register(c150);
float4 FalloffFactorE : register(c151);
float4 Luminance : register(c152);

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    return pow(abs(tex2D(diffuseSampler, UV(0))), GAMMA);
}

float4 GetSpecular(DECLARATION_TYPE input)
{
    float4 specular = tex2D(specularSampler, UV(1));
    return float4(specular.x * 0.25, specular.yzw);
}

float3 GetNormal(DECLARATION_TYPE input, float3x3 tangentToWorldMatrix)
{
    float4 normal = tex2D(normalSampler, UV(2));

    normal.xy = normal.xy * 2 - 1;
    normal.z = sqrt(1 - saturate(dot(normal.xy, normal.xy)));

    return mul(tangentToWorldMatrix, normal.yxz);
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
    float factor = ComputeFalloff(material.CosViewDirection, FalloffFactor.xyz);

    float3 falloff = pow(tex2D(falloffSampler, UV(4)), GAMMA).xyz;

    material.Albedo = lerp(material.Albedo, falloff * FalloffFactor.www, factor);
    material.F0 *= saturate(tex2D(reflectionSampler, float2(0.5, input.Color.w)).x + g_Specular.x);
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
    finalColor.rgb += ComputeFalloff(material.CosViewDirection, 
        float3(FalloffFactorE.x, FalloffFactorE.y * g_Ambient.x, FalloffFactorE.z)) * g_Ambient.rgb * Luminance.xxx;
}

#endif