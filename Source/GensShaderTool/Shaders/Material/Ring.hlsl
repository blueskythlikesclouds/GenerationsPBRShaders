#ifndef SCENE_MATERIAL_RING_HLSL_INCLUDED
#define SCENE_MATERIAL_RING_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#define DECLARATION_TYPE    DefaultDeclaration

sampler2D diffuseSampler : register(s0);
sampler2D emissionSampler : register(s1);

float4 PBRFactor : register(c150);
float4 Blend : register(c151);

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    return pow(abs(tex2D(diffuseSampler, UV(0))), GAMMA) * float4(input.Color.xyz, 1.0);
}

float4 GetSpecular(DECLARATION_TYPE input)
{
    return float4(PBRFactor.xy, 1, 1);
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
    float3 viewNormal = normalize(mul(float4(material.Normal, 0), g_MtxView).xyz);
    float3 emission = pow(tex2D(emissionSampler, viewNormal * float2(0.5, -0.5) + 0.5), GAMMA).rgb * input.Color.xyz;

    finalColor.rgb += emission * (1 - Blend.x) * mrgLuminanceRange.x;
}

#endif