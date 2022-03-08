#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 PBRFactor : packoffset(c150);
float4 Luminance : packoffset(c151);
float4 FalloffFactor : packoffset(c152);
float4 Refraction : packoffset(c153);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texSpecular : register(t1);
Texture2D<float4> texNormal : register(t2);
Texture2D<float4> texEmission : register(t3);

SamplerState sampDiffuse : register(s0);
SamplerState sampSpecular : register(s1);
SamplerState sampNormal : register(s2);
SamplerState sampEmission : register(s3);

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

#ifdef HasSamplerEmission
    params.Emission = texEmission.Sample(sampEmission, UV(3)).rgb * g_Ambient.rgb * Luminance.x;
#endif
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input)
{
    float factor = ComputeFalloff(params.CosViewDirection, FalloffFactor.xyz);

    params.Reflectance += (1 - params.Reflectance) * factor;
    params.Refraction.w = 1.0 - (params.Alpha + (1 - params.Alpha) * factor);
    params.Alpha = 1.0;

    params.Refraction.rgb = params.Albedo * g_FramebufferTexture.SampleLevel(g_LinearClampSampler, input.SVPosition.xy * g_ViewportSize.zw, 0).rgb;
}

#include "../DefaultPS.hlsli"