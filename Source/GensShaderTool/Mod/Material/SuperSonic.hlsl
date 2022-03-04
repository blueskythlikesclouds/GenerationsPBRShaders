#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 Blend : packoffset(c150);
float4 FalloffFactor : packoffset(c151);

GLOBALS_PS_APPEND_PARAMETERS_END

#include "../SharedPS.hlsli"

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texEmission : register(t1);
Texture2D<float4> texSpecular : register(t2);
Texture2D<float4> texNormal : register(t3);
Texture2D<float4> texFalloff : register(t4);

SamplerState sampDiffuse : register(s0);
SamplerState sampEmission : register(s1);
SamplerState sampSpecular : register(s2);
SamplerState sampNormal : register(s3);
SamplerState sampFalloff : register(s4);

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    float4 diffuse = texDiffuse.Sample(sampDiffuse, UV(0));

    params.Albedo = SrgbToLinear(diffuse.rgb) * input.Color.rgb;
    params.Alpha = diffuse.a * input.Color.a;

    ConvertSpecularToParams(texSpecular.Sample(sampSpecular, UV(2)), false, params);

    params.NormalMap = texNormal.Sample(sampNormal, UV(3)).xy;
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input)
{
    float3 emission = SrgbToLinear(texEmission.Sample(sampEmission, UV(1)).rgb);
    float lerpAmount = ComputeFalloff(params.CosViewDirection, FalloffFactor.xyz);

#ifdef HasSamplerFalloff
    float4 falloff = SrgbToLinear(texFalloff.Sample(sampFalloff, UV(4)));
    lerpAmount *= falloff.w;
    emission = lerp(emission, falloff, lerpAmount);
#endif

    params.Emission = emission * max(Blend.x, lerpAmount) * GetToneMapLuminance();
}

#include "../DefaultPS.hlsli"