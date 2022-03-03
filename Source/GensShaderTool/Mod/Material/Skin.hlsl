#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 PBRFactor : packoffset(c150);
float4 FalloffFactor : packoffset(c151);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texSpecular : register(t1);
Texture2D<float4> texNormal : register(t2);
Texture2D<float4> texCdr : register(t3);
Texture2D<float4> texFalloff : register(t4);

SamplerState sampDiffuse : register(s0);
SamplerState sampSpecular : register(s1);
SamplerState sampNormal : register(s2);
SamplerState sampCdr : register(s3);
SamplerState sampFalloff : register(s4);

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    float4 diffuse = texDiffuse.Sample(sampDiffuse, UV(0));

    params.Albedo = SrgbToLinear(diffuse.rgb);
    params.Alpha = diffuse.a;

#ifdef HasSamplerSpecular
    ConvertSpecularToParams(texSpecular.Sample(sampSpecular, UV(1)), false, params);
#else
    ConvertPBRFactorToParams(PBRFactor, params);
#endif

    params.NormalMap = texNormal.Sample(sampNormal, UV(2)).xy;
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input)
{
    params.Cdr = texCdr.Sample(sampCdr, float2(dot(params.Normal, -mrgGlobalLight_Direction.xyz) * 0.5 + 0.5, input.Color.y));

    float3 falloff = SrgbToLinear(texFalloff.Sample(sampFalloff, UV(4)).rgb);

#ifdef HasSamplerSpecular
    falloff *= texSpecular.Sample(sampSpecular, UV(1)).w;
#endif

    falloff *= FalloffFactor.w;

    float lerpAmount = ComputeFalloff(params.CosViewDirection, FalloffFactor.xyz);
    params.Albedo = lerp(params.Albedo, falloff, lerpAmount);
}

#include "../DefaultPS.hlsli"