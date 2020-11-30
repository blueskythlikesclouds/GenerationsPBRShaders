#ifndef SCENE_MATERIAL_BLEND_HLSL_INCLUDED
#define SCENE_MATERIAL_BLEND_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#include "Default.hlsl"

sampler2D diffuseSampler : register(s0);
sampler2D specularSampler : register(s1);
sampler2D normalSampler : register(s2);
sampler2D blendSampler : register(s3);
sampler2D diffuseBlendSampler : register(s4);
sampler2D specularBlendSampler : register(s5);
sampler2D normalBlendSampler : register(s6);

float4 PBRFactor : register(c150);

float GetBlend(DECLARATION_TYPE input)
{
#if defined(HasBlend) && HasBlend
    return tex2D(blendSampler, UV(3)).x; // this better get optimized...
#else
    return input.Color.w;
#endif
}

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    float4 s0 = pow(abs(tex2D(diffuseSampler, UV(0))), GAMMA);
    float4 s1 = pow(abs(tex2D(diffuseBlendSampler, UV(4))), GAMMA);

    return lerp(s0, s1, GetBlend(input)) * float4(input.Color.xyz, 1);
}

float4 GetSpecular(DECLARATION_TYPE input)
{
#if defined(HasSpecular) && HasSpecular
    float4 specular = tex2D(specularSampler, UV(1));

#if defined(HasSpecularBlend) && HasSpecularBlend
    specular = lerp(specular, tex2D(specularBlendSampler, UV(5)), GetBlend(input));
#endif

    return float4(specular.x * 0.25, specular.yzw);

#else
    return float4(PBRFactor.xy, 1, 1);
#endif
}

float3 GetNormal(DECLARATION_TYPE input, float3x3 tangentToWorldMatrix)
{
    float4 normal = tex2D(normalSampler, UV(2));

#if defined(HasNormalBlend) && HasNormalBlend
    normal = lerp(normal, tex2D(normalBlendSampler, UV(6)), GetBlend(input));
#endif

    normal.xy = normal.xy * 2 - 1;
    normal.z = sqrt(1 - saturate(dot(normal.xy, normal.xy)));

    return mul(tangentToWorldMatrix, normal.yxz);
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
}

#endif