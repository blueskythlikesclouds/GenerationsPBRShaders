#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 PBRFactor : packoffset(c150);
float4 FalloffFactor : packoffset(c151);
float4 MaxOpacity : packoffset(c152);

GLOBALS_PS_APPEND_PARAMETERS_END

#include "../SharedPS.hlsli"

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texSpecular : register(t1);
Texture2D<float4> texNormal : register(t2);
Texture2D<float4> texFalloff : register(t3);

SamplerState sampDiffuse : register(s0);
SamplerState sampSpecular : register(s1);
SamplerState sampNormal : register(s2);
SamplerState sampFalloff : register(s3);

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    float4 diffuse = texDiffuse.Sample(sampDiffuse, UV(0));

    params.Albedo = SrgbToLinear(diffuse.rgb) * input.Color.rgb;
    params.Alpha = diffuse.a * input.Color.a;

#ifdef HasSamplerSpecular
    ConvertSpecularToParams(texSpecular.Sample(sampSpecular, UV(1)), false, params);
#else
    ConvertPBRFactorToParams(PBRFactor, params);
#endif

    params.NormalMap = texNormal.Sample(sampNormal, UV(2)).xy;
    params.Emission += g_Emissive.rgb * g_Ambient.rgb * GetToneMapLuminance();
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input)
{
    float factor = ComputeFalloff(params.CosViewDirection, FalloffFactor.xyz);

    params.Alpha = min(saturate(params.Alpha + factor), max(MaxOpacity.x, params.Alpha));

    float3 falloff = 1.0;

#ifdef HasSamplerFalloff
    falloff = SrgbToLinear(texFalloff.Sample(sampFalloff, UV(3)).rgb);
#endif

    params.Albedo = lerp(params.Albedo, falloff * FalloffFactor.w, factor);
}

#include "../DefaultPS.hlsli"