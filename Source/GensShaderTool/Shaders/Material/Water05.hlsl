#ifndef SCENE_MATERIAL_WATER05_HLSL_INCLUDED
#define SCENE_MATERIAL_WATER05_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#undef DECLARATION_TYPE
#define DECLARATION_TYPE    WaterDeclaration

sampler2D diffuseSampler : register(s0);
sampler2D specularSampler : register(s1);
sampler2D normalSampler : register(s2);
sampler2D normal1Sampler : register(s3);

float4 RefractionCubemap : register(c150);

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    return pow(abs(tex2D(diffuseSampler, UV(0) + material.Normal.xy * RefractionCubemap.z)), GAMMA);
}

float4 GetSpecular(DECLARATION_TYPE input)
{
    float4 specular = tex2D(specularSampler, UV(1));
    return float4(specular.x * 0.25, specular.yzw);
}

float3 GetNormal(DECLARATION_TYPE input, float3x3 tangentToWorldMatrix)
{
    float4 n0 = tex2D(normalSampler, UV(2));

    n0.xy = n0.xy * 2 - 1;
    n0.z = sqrt(1 - dot(n0.xy, n0.xy));

    float3 normal = mul(tangentToWorldMatrix, n0.yxz);

    float4 n1 = tex2D(normal1Sampler, UV(3));

    n1.xy = n1.xy * 2 - 1;
    n1.z = sqrt(1 - dot(n1.xy, n1.xy));

    float3 normal1 = mul(tangentToWorldMatrix, n1.yxz);

    return lerp(normal, normal1, 1 - RefractionCubemap.x);
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
    if (isDeferred)
        return;

    float2 texCoord = input.SvPosition.xy + mul(float4(material.Normal, 0), g_MtxView).xy * RefractionCubemap.y * g_ViewportSize.zw;

    float depth = tex2Dproj(g_DepthSampler, float4(texCoord, 0, input.SvPosition.w)).x;
    if (depth >= input.SvPosition.z / input.SvPosition.w)
        texCoord = input.SvPosition.xy;

    float4 color = tex2Dproj(g_FramebufferSampler, float4(texCoord, 0, input.SvPosition.w));

    finalColor = float4(lerp(finalColor, color.rgb * input.Color.rgb, finalColor.a), input.Color.a);
}

#endif