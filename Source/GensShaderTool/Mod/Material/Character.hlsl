#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

#include "../SharedPS.hlsli"

float4 PBRFactor : packoffset(c150);
float4 Luminance : packoffset(c151);
float4 FalloffFactor : packoffset(c152);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texEmission : register(t1);
Texture2D<float4> texSpecular : register(t2);
Texture2D<float4> texNormal : register(t3);
Texture2D<float4> texCdr : register(t4);
Texture2D<float4> texFalloff : register(t5);

SamplerState sampDiffuse : register(s0);
SamplerState sampEmission : register(s1);
SamplerState sampSpecular : register(s2);
SamplerState sampNormal : register(s3);
SamplerState sampCdr : register(s4);
SamplerState sampFalloff : register(s5);

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    float4 diffuse = texDiffuse.Sample(sampDiffuse, UV(0));

    params.Albedo = SrgbToLinear(diffuse.rgb);
    params.Alpha = diffuse.a;

#ifndef HasSamplerCdr
    params.Albedo *= input.Color.rgb;
    params.Alpha *= input.Color.a;
#endif

#ifdef HasSamplerSpecular
    ConvertSpecularToParams(texSpecular.Sample(sampSpecular, UV(2)), HasExplicitMetalness, params);
#else
    ConvertPBRFactorToParams(PBRFactor, params);
#endif

    params.NormalMap = texNormal.Sample(sampNormal, UV(2)).xy;

#ifdef HasSamplerEmission
    params.Emission = texEmission.Sample(sampEmission, UV(1)) * g_Ambient.rgb * Luminance.x * GetToneMapLuminance();
#endif
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input)
{
#ifdef HasSamplerCdr
    params.Cdr = texCdr.Sample(sampCdr, float2(dot(params.Normal, -mrgGlobalLight_Direction.xyz) * 0.5 + 0.5, input.Color.y));
#endif

#ifdef HasSamplerFalloff
    float3 falloff = SrgbToLinear(texFalloff.Sample(sampFalloff, UV(5)).rgb);
    falloff *= FalloffFactor.w;

    float lerpAmount = ComputeFalloff(params.CosViewDirection, FalloffFactor.xyz);

#ifdef HasSamplerSpecular
    lerpAmount *= texSpecular.Sample(sampSpecular, UV(2)).w;
#endif

    params.Albedo = lerp(params.Albedo, falloff, lerpAmount);
#endif
}

#include "../DefaultPS.hlsli"