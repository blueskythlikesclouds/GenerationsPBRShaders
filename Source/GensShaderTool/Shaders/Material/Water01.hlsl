#ifndef SCENE_MATERIAL_WATER01_HLSL_INCLUDED
#define SCENE_MATERIAL_WATER01_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#include "Default.hlsl"

sampler2D diffuseSampler : register(s0);
sampler2D normalSampler : register(s1);
sampler2D normal1Sampler : register(s2);

float4 PBRFactor : register(c150);

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    return pow(abs(tex2D(diffuseSampler, UV(0))), GAMMA);
}

float4 GetSpecular(DECLARATION_TYPE input)
{
    return float4(PBRFactor.xy, 1, 1);
}

float3 GetNormal(DECLARATION_TYPE input, float3x3 tangentToWorldMatrix)
{
    float4 n0 = tex2D(normalSampler, UV(1));

    n0.xy = n0.xy * 2 - 1;
    n0.z = sqrt(1 - dot(n0.xy, n0.xy));

    float3 normal = mul(tangentToWorldMatrix, n0.yxz);

    float4 n1 = tex2D(normal1Sampler, UV(2));

    n1.xy = n1.xy * 2 - 1;
    n1.z = sqrt(1 - dot(n1.xy, n1.xy));

    float3 normal1 = mul(tangentToWorldMatrix, n1.yxz);

    return lerp(normal, normal1, input.Color.w);
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, inout float4 finalColor)
{
}

#endif