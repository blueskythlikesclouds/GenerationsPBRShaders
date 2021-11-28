#ifndef SCENE_MATERIAL_TRIPLANAR_HLSL_INCLUDED
#define SCENE_MATERIAL_TRIPLANAR_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#include "Default.hlsl"

float4 Scale : register(c150);
float4 Scale2 : register(c151);

sampler2D diffuseSampler : register(s0);
sampler2D specularSampler : register(s1);
sampler2D normalSampler : register(s2);

sampler2D diffuseBlendSampler : register(s3);
sampler2D specularBlendSampler : register(s4);
sampler2D normalBlendSampler : register(s5);

sampler2D normalDetailSampler : register(s6);

float GetScale(int index)
{
    if (index >= 0 && index <= 2)
        return Scale[index];

    if (index >= 3 && index <= 6)
        return Scale2[index - 3];

    return 0.0;
}

float4 SampleTex(DECLARATION_TYPE input, sampler2D tex, int index)
{
    float scale = GetScale(index);

    float4 u = tex2D(tex, scale == 0.0 ? UV(index) : input.Position.zy * scale);
    float4 v = tex2D(tex, scale == 0.0 ? UV(index) : input.Position.xz * scale);
    float4 w = tex2D(tex, scale == 0.0 ? UV(index) : input.Position.xy * scale);

    float3 blend = normalize(input.Normal);
    blend *= blend;

    return blend.x * u + blend.y * v + blend.z * w;
}

float3 SampleNormal(DECLARATION_TYPE input, sampler2D tex, int index)
{
    float4 normal = SampleTex(input, tex, index);
    normal.xy = normal.xy * 2.0 - 1.0;
    normal.z = sqrt(1.0 - saturate(dot(normal.xy, normal.xy)));
    return normal.xyz;
}

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    float4 s0 = pow(abs(SampleTex(input, diffuseSampler, 0)), GAMMA);
    float4 s1 = pow(abs(SampleTex(input, diffuseBlendSampler, 3)), GAMMA);

    return lerp(s0, s1, input.Color.w) * float4(input.Color.xyz, 1);
}

float4 GetSpecular(DECLARATION_TYPE input)
{
    return lerp(
        SampleTex(input, specularSampler, 1),
        SampleTex(input, specularBlendSampler, 4), input.Color.w) * float4(0.25, 1, 1, 1);
}

float3 GetNormal(DECLARATION_TYPE input, float3x3 tangentToWorldMatrix)
{
    float3 normal = SampleNormal(input, normalSampler, 2);

#if defined(HasNormalBlend) && HasNormalBlend
    float3 normalBlend = SampleNormal(input, normalBlendSampler, 5);

#if defined(HasNormalDetail) && HasNormalDetail
    float3 normalDetail = SampleNormal(input, normalDetailSampler, 6);
    normalBlend = BlendNormal(normalBlend, normalDetail);
#endif

    normal = normalize(lerp(normal, normalBlend, input.Color.w));
#endif

    return mul(tangentToWorldMatrix, normal);
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
}

#endif