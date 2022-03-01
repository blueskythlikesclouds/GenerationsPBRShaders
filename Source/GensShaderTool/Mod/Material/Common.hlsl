#include "../Shared.hlsli"
#include "../Utilities.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 PBRFactor : packoffset(c150);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texSpecular : register(t1);
Texture2D<float4> texNormal : register(t2);
Texture2D<float4> texTransparency : register(t3);

SamplerState sampDiffuse : register(s0);
SamplerState sampSpecular : register(s1);
SamplerState sampNormal : register(s2);
SamplerState sampTransparency : register(s3);

void LoadParams(in PixelDeclaration input, inout ShaderParams params)
{
    float4 diffuse = texDiffuse.Sample(sampDiffuse, UV(0));

    params.Albedo = SrgbToLinear(diffuse.rgb);
    params.Alpha = diffuse.a;

#if defined(HasSamplerSpecular)
    ConvertSpecularToParams(texSpecular.Sample(sampSpecular, UV(1)), HasExplicitMetalness, params);
#else
    ConvertPBRFactorToParams(PBRFactor, params);
#endif

    params.NormalMap = texNormal.Sample(sampNormal, UV(2)).xy;

#if defined(HasSamplerTransparency)
    params.Alpha *= texTransparency.Sample(sampTransparency, UV(3)).a;
#endif
}

void ModifyParams(in PixelDeclaration input, inout ShaderParams params) {}

#include "../DefaultPS.hlsli"