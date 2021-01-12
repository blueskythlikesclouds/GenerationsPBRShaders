#ifndef SCENE_MATERIAL_GLASS_HLSL_INCLUDED
#define SCENE_MATERIAL_GLASS_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#include "Default.hlsl"

sampler2D diffuseSampler : register(s0);
sampler2D specularSampler : register(s1);
sampler2D normalSampler : register(s2);
sampler2D emissionSampler : register(s3);

float4 PBRFactor : register(c150);
float4 FalloffFactor : register(c151);
float4 Refraction : register(c152);
float4 Luminance : register(c153);

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    return pow(abs(tex2D(diffuseSampler, UV(0))), GAMMA) * input.Color;
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
    float factor = ComputeFalloff(material.CosViewDirection, FalloffFactor.xyz);

    material.Alpha += (1 - material.Alpha) * factor;
    material.F0 += (1 - material.F0) * factor;
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
    if (isDeferred)
        return;

    // TODO: Handle refraction parallax
    float4 framebuffer = tex2Dlod(g_FramebufferSampler, float4((input.VPos.xy + 0.5) * g_ViewportSize.zw, 0, 0));

    finalColor.rgb = lerp(framebuffer.rgb * material.Albedo, finalColor.rgb, finalColor.a);
    finalColor.a = 1.0;

#if defined(HasEmission) && HasEmission
    finalColor.rgb += tex2D(emissionSampler, UV(3)).rgb * g_Ambient.rgb * Luminance.x;
#endif
}

#endif