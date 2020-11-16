#ifndef SCENE_MATERIAL_SKIN_HLSL_INCLUDED
#define SCENE_MATERIAL_SKIN_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#include "Default.hlsl"

sampler2D diffuseSampler : register(s0);
sampler2D specularSampler : register(s1);
sampler2D normalSampler : register(s2);
sampler2D cdrSampler : register(s3);
sampler2D falloffSampler : register(s4);

float4 PBRFactor : register(c150);
float4 FalloffFactor : register(c151);

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    return pow(abs(tex2D(diffuseSampler, UV(0))), GAMMA);
}

float4 GetSpecular(DECLARATION_TYPE input)
{
#if defined(HasSpecular) && HasSpecular
    return tex2D(specularSampler, UV(1));
#else
    return float4(PBRFactor.xy, 1, 1);
#endif
}

float3 GetNormal(DECLARATION_TYPE input, float3x3 tangentToWorldMatrix)
{
    float4 normal = tex2D(normalSampler, UV(2));

    normal.xy = normal.xy * 2 - 1;
    normal.z = sqrt(1 - normal.x * normal.x - normal.y * normal.y);

    return mul(tangentToWorldMatrix, normal.yxz);
}

float3 GetCdr(float cosTheta, float curvature)
{
    return tex2D(cdrSampler, float2(cosTheta * 0.5 + 0.5, curvature));
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
    float factor = exp2(log2(saturate(1 - material.CosViewDirection + FalloffFactor.z)) * FalloffFactor.y) * FalloffFactor.x;

    float3 falloff = pow(tex2D(falloffSampler, UV(4)), GAMMA).xyz;

    material.Albedo = lerp(material.Albedo, falloff * FalloffFactor.www, factor);
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, inout float4 finalColor)
{
}

#endif