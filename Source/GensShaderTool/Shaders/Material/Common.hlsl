#ifndef SCENE_MATERIAL_COMMON_HLSL_INCLUDED
#define SCENE_MATERIAL_COMMON_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#include "Default.hlsl"

sampler2D diffuseSampler : register(s0);
sampler2D specularSampler : register(s1);
sampler2D normalSampler : register(s2);
sampler2D transparencySampler : register(s3);

float4 PBRFactor : register(c150);

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    float4 diffuse = pow(abs(tex2D(diffuseSampler, UV(0))), GAMMA) * input.Color;

#if defined(HasTransparency) && HasTransparency
    diffuse.a *= tex2D(transparencySampler, UV(3)).a;
#endif

    return diffuse;
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
    normal.z = sqrt(1 - dot(normal.xy, normal.xy));

    return mul(tangentToWorldMatrix, normal.yxz);
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
}

#endif