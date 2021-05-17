#ifndef SCENE_MATERIAL_SUPER_SONIC_HLSL_INCLUDED
#define SCENE_MATERIAL_SUPER_SONIC_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#include "Default.hlsl"

sampler2D diffuseSampler : register(s0);
sampler2D emissionSampler : register(s1);
sampler2D specularSampler : register(s2);
sampler2D normalSampler : register(s3);
sampler2D falloffSampler : register(s4);

float4 Blend : register(c150);
float4 FalloffFactor : register(c151);

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    return pow(abs(tex2D(diffuseSampler, UV(0))), GAMMA) * input.Color;
}

float4 GetSpecular(DECLARATION_TYPE input)
{
    float4 specular = tex2D(specularSampler, UV(2));
    return float4(specular.x * 0.25, specular.yzw);
}

float3 GetNormal(DECLARATION_TYPE input, float3x3 tangentToWorldMatrix)
{
    float4 normal = tex2D(normalSampler, UV(3));

    normal.xy = normal.xy * 2 - 1;
    normal.z = sqrt(1 - saturate(dot(normal.xy, normal.xy)));

    return mul(tangentToWorldMatrix, normal.xyz);
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
    float3 emission = pow(abs(tex2D(emissionSampler, UV(2))), GAMMA).rgb;
    float factor = ComputeFalloff(material.CosViewDirection, FalloffFactor.xyz);

#if defined(HasFalloff) && HasFalloff
    float4 falloff = pow(tex2D(falloffSampler, UV(4)), GAMMA);
    factor *= falloff.a;
    emission = lerp(emission, falloff, factor);
#endif

    finalColor.rgb += emission * max(Blend.x, factor) * GetToneMapLuminance();
}

#endif