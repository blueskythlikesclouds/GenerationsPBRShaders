#ifndef SCENE_MATERIAL_CHR_EMISSION_HLSL_INCLUDED
#define SCENE_MATERIAL_CHR_EMISSION_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#include "Default.hlsl"

sampler2D diffuseSampler : register(s0);
sampler2D specularSampler : register(s1);
sampler2D normalSampler : register(s2);
sampler2D falloffSampler : register(s3);
sampler2D emissionSampler : register(s4);
sampler2D transparencySampler : register(s5);

float4 PBRFactor : register(c150);
float4 Luminance : register(c151);
float4 FalloffFactor : register(c152);

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    return pow(abs(tex2D(diffuseSampler, UV(0))), GAMMA) * input.Color;
}

float4 GetSpecular(DECLARATION_TYPE input)
{
#if defined(HasSpecular) && HasSpecular
    float4 specular = tex2D(specularSampler, UV(1));
    return float4(specular.x * 0.25, specular.yzw);
#else
    return float4(PBRFactor.xy, 1, 1);
#endif
}

float3 GetNormal(DECLARATION_TYPE input, float3x3 tangentToWorldMatrix)
{
    float4 normal = tex2D(normalSampler, UV(2));

    normal.xy = normal.xy * 2 - 1;
    normal.z = sqrt(1 - saturate(dot(normal.xy, normal.xy)));

    return mul(tangentToWorldMatrix, normal.xyz);
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
#if defined(HasFalloff) && HasFalloff
    float factor = ComputeFalloff(material.CosViewDirection, FalloffFactor.xyz);
    float3 falloff = pow(tex2D(falloffSampler, UV(3)), GAMMA).xyz;
    material.Albedo = lerp(material.Albedo, falloff * FalloffFactor.www, factor);
#endif
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
    float3 emission = tex2D(emissionSampler, UV(4)).rgb * g_Ambient.rgb * Luminance.x;

#if defined(HasTransparency) && HasTransparency
    emission *= tex2D(transparencySampler, UV(5)).a;
#endif

    finalColor.rgb += emission * GetToneMapLuminance();
}

#endif