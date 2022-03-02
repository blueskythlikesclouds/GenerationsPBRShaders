#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 PBRFactor : packoffset(c150);
float4 PBRFactor2 : packoffset(c151);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texSpecular : register(t1);
Texture2D<float4> texNormal : register(t2);
Texture2D<float> texBlend : register(t3);
Texture2D<float4> texDiffuseBlend : register(t4);
Texture2D<float4> texSpecularBlend : register(t5);
Texture2D<float4> texNormalBlend : register(t6);

SamplerState sampDiffuse : register(s0);
SamplerState sampSpecular : register(s1);
SamplerState sampNormal : register(s2);
SamplerState sampBlend : register(s3);
SamplerState sampDiffuseBlend : register(s4);
SamplerState sampSpecularBlend : register(s5);
SamplerState sampNormalBlend : register(s6);

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
#ifdef HasSamplerBlend
    float blend = texBlend.Sample(sampBlend, UV(3));
#else
    float blend = input.Color.w;
#endif

    float4 diffuse = lerp(
        SrgbToLinear(texDiffuse.Sample(sampDiffuse, UV(0))), 
        SrgbToLinear(texDiffuseBlend.Sample(sampDiffuseBlend, UV(4))), blend);

    params.Albedo = diffuse.rgb * input.Color.rgb;
    params.Alpha = diffuse.a;

#ifdef HasSamplerBlend
    params.Alpha *= input.Color.a;
#endif

#ifdef HasSamplerSpecular
    float4 specular = texSpecular.Sample(sampSpecular, UV(1));

#ifdef HasSamplerSpecularBlend
    specular = lerp(specular, texSpecularBlend.Sample(sampSpecularBlend, UV(5)), blend);
#endif

    ConvertSpecularToParams(specular, HasExplicitMetalness, params);
#else
    ConvertPBRFactorToParams(lerp(PBRFactor, PBRFactor2, blend), params);
#endif

    params.NormalMap = texNormal.Sample(sampNormal, UV(2)).xy;

#ifdef HasSamplerNormalBlend
    params.NormalMap = lerp(params.NormalMap, texNormalBlend.Sample(sampNormalBlend, UV(6)).xy, blend);
#endif
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input) {}

#include "../DefaultPS.hlsli"