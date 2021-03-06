#ifndef SCENE_MATERIAL_EYE_HLSL_INCLUDED
#define SCENE_MATERIAL_EYE_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#undef DECLARATION_TYPE
#define DECLARATION_TYPE EyeDeclaration

sampler2D diffuseSampler : register(s0);
sampler2D specularSampler : register(s1);
sampler2D cdrSampler : register(s2);
sampler2D reflectionSampler : register(s3);

float4 ChrEye1 : register(c150);
float4 ChrEye2 : register(c151);
float4 ChrEye3 : register(c152);

#if defined(IsChrEyeSuper) && IsChrEyeSuper
float4 Blend : register(c153);
#endif

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    return pow(abs(tex2D(diffuseSampler, UV(0))), GAMMA);
}

float4 GetSpecular(DECLARATION_TYPE input)
{
    float4 specular = tex2D(specularSampler, UV(1));
    return float4(specular.x * 0.25, specular.yzw);
}

float3 GetCdr(float cosTheta, float curvature)
{
    return tex2Dlod(cdrSampler, float4(cosTheta * 0.5 + 0.5, curvature, 0, 0));
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
    float3 eyeNormal = normalize(input.EyeNormal);
    float2 directionOffset = float2(dot(eyeNormal, float3(-1, 0, 0)), dot(eyeNormal, float3(0, 1, 0)));

    float2 diffuseAlphaOffset = -(ChrEye1.zw * directionOffset);
    float2 reflectionOffset = ChrEye3.xy + ChrEye3.zw * directionOffset;
    float2 reflectionAlphaOffset = ChrEye1.xy - float2(directionOffset.x < 0 ? ChrEye2.x : ChrEye2.y, directionOffset.y < 0 ? ChrEye2.z : ChrEye2.w) * directionOffset;

    float diffuseAlpha = tex2D(diffuseSampler, UV(0) + diffuseAlphaOffset).a;
    float3 reflection = tex2D(reflectionSampler, UV(0) + reflectionOffset).rgb;
    float3 reflectionAlpha = tex2D(reflectionSampler, UV(3) + reflectionAlphaOffset).a;

    material.Albedo = lerp((material.Albedo + reflection * material.Metalness) * diffuseAlpha, 1, reflectionAlpha);
    material.Alpha = 1;
    material.AmbientOcclusion = lerp(material.AmbientOcclusion, 1, reflectionAlpha);
    material.Metalness = 0;
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
#if defined(IsChrEyeSuper) && IsChrEyeSuper
    finalColor.rgb += material.Albedo * Blend.x * GetToneMapLuminance();
#endif
}

#endif