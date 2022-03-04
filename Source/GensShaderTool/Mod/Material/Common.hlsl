#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 PBRFactor : packoffset(c150);
float4 Luminance : packoffset(c151);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texSpecular : register(t1);
Texture2D<float4> texNormal : register(t2);
Texture2D<float4> texEmission : register(t3);
Texture2D<float> texTransparency : register(t4);

SamplerState sampDiffuse : register(s0);
SamplerState sampSpecular : register(s1);
SamplerState sampNormal : register(s2);
SamplerState sampEmission : register(s3);
SamplerState sampTransparency : register(s4);

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    float4 diffuse = texDiffuse.Sample(sampDiffuse, UV(0));

    params.Albedo = SrgbToLinear(diffuse.rgb) * input.Color.rgb;
    params.Alpha = diffuse.a * input.Color.a;

#ifdef HasSamplerSpecular
    ConvertSpecularToParams(texSpecular.Sample(sampSpecular, UV(1)), HasExplicitMetalness, params);
#else
    ConvertPBRFactorToParams(PBRFactor, params);
#endif

    params.NormalMap = texNormal.Sample(sampNormal, UV(2)).xy;

#ifdef HasSamplerEmission
    params.Emission = texEmission.Sample(sampEmission, UV(3)).rgb * g_Ambient.rgb * Luminance.x;
#endif

#ifdef HasSamplerTransparency
    float transparency = texTransparency.Sample(sampTransparency, UV(3));

#ifdef HasSamplerEmission
    params.Emission *= transparency;
#else
    params.Alpha *= transparency;
#endif

#endif
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input) {}

#include "../DefaultPS.hlsli"