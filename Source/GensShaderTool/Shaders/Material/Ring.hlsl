#ifndef SCENE_MATERIAL_RING_HLSL_INCLUDED
#define SCENE_MATERIAL_RING_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#include "Default.hlsl"

sampler2D diffuseSampler : register(s0);
sampler2D emissionSampler : register(s1);
sampler2D specularSampler : register(s2);
sampler2D normalSampler : register(s3);

float4 PBRFactor : register(c150);
float4 Blend : register(c151);

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    return pow(abs(tex2D(diffuseSampler, UV(0))), GAMMA) * float4(input.Color.xyz, 1.0);
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
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
    float3 viewNormal = normalize(mul(float4(material.Normal, 0), g_MtxView).xyz);
    float3 emission = pow(tex2D(emissionSampler, viewNormal * float2(0.5, -0.5) + 0.5), GAMMA).rgb * input.Color.xyz;

    finalColor.rgb += emission * (1 - Blend.x) * g_HDRParam_SGGIParam.x;
}

#endif