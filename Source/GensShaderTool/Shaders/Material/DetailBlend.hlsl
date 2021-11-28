#ifndef SCENE_MATERIAL_DETAIL_BLEND_HLSL_INCLUDED
#define SCENE_MATERIAL_DETAIL_BLEND_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#include "Default.hlsl"

float4 TileScale : register(c150);
float4 TileScale2 : register(c151);

sampler2D diffuseSampler : register(s0);
sampler2D specularSampler : register(s1);
sampler2D normalSampler : register(s2);

sampler2D diffuseBlendSampler : register(s3);
sampler2D specularBlendSampler : register(s4);
sampler2D normalBlendSampler : register(s5);

sampler2D normalDetailSampler : register(s6);

float GetTileScale(int index)
{
    if (index >= 0 && index <= 2)
        return TileScale[index];

    if (index >= 3 && index <= 6)
        return TileScale2[index - 3];

    return 0.0;
}

float4 SampleTex(DECLARATION_TYPE input, sampler2D tex, int index)
{
    float tileScale = GetTileScale(index);

    float4 u = tex2D(tex, tileScale == 0.0 ? UV(index) : input.Position.zy * tileScale);
    float4 v = tex2D(tex, tileScale == 0.0 ? UV(index) : input.Position.xz * tileScale);
    float4 w = tex2D(tex, tileScale == 0.0 ? UV(index) : input.Position.xy * tileScale);

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
    float3 normalBlend = SampleNormal(input, normalBlendSampler, 5);
    float3 normalDetail = SampleNormal(input, normalDetailSampler, 6);

    return mul(tangentToWorldMatrix, normalize(lerp(normal.xyz, BlendNormal(normalBlend, normalDetail), input.Color.w)));
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
}

#endif