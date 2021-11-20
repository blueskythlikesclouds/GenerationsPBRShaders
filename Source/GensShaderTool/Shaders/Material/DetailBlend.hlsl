#ifndef SCENE_MATERIAL_DETAIL_BLEND_HLSL_INCLUDED
#define SCENE_MATERIAL_DETAIL_BLEND_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#include "Default.hlsl"

// Grass (TexCoord0.xy)
sampler2D diffuseSampler : register(s0);
sampler2D specularSampler : register(s1);
sampler2D normalSampler : register(s2);

// Rock (TexCoord1.xy)
sampler2D diffuseBlendSampler : register(s3);
sampler2D specularBlendSampler : register(s4);
sampler2D normalBlendSampler : register(s5);

// Rock Detail (TexCoord1.zw)
sampler2D normalDetailSampler : register(s6);

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    float4 s0 = pow(abs(tex2D(diffuseSampler, input.TexCoord0.xy)), GAMMA);
    float4 s1 = pow(abs(tex2D(diffuseBlendSampler, input.TexCoord1.xy)), GAMMA);

    return lerp(s0, s1, input.Color.w) * float4(input.Color.xyz, 1);
}

float4 GetSpecular(DECLARATION_TYPE input)
{
    return lerp(
        tex2D(specularSampler, input.TexCoord0.xy), 
        tex2D(specularBlendSampler, input.TexCoord1.xy), input.Color.w) * float4(0.25, 1, 1, 1);
}

float3 GetNormal(DECLARATION_TYPE input, float3x3 tangentToWorldMatrix)
{
    float3 normal = SampleNormal(normalSampler, input.TexCoord0.xy);
    float3 normalBlend = SampleNormal(normalBlendSampler, input.TexCoord1.xy);
    float3 normalDetail = SampleNormal(normalDetailSampler, input.TexCoord1.zw);

    return mul(tangentToWorldMatrix, normalize(lerp(normal.xyz, BlendNormal(normalBlend, normalDetail), input.Color.w)));
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
}

#endif