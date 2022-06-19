#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 PBRFactor : packoffset(c150);
float4 Blend : packoffset(c151);

GLOBALS_PS_APPEND_PARAMETERS_END

#include "../SharedPS.hlsli"

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texEmission : register(t1);
Texture2D<float4> texSpecular : register(t2);
Texture2D<float4> texNormal : register(t3);

SamplerState sampDiffuse : register(s0);
SamplerState sampEmission : register(s1);
SamplerState sampSpecular : register(s2);
SamplerState sampNormal : register(s3);

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    float4 diffuse = texDiffuse.Sample(sampDiffuse, UV(0));

    params.Albedo = SrgbToLinear(diffuse.rgb) * input.Color.rgb;
    params.Alpha = diffuse.a * input.Color.a;

#ifdef HasSamplerSpecular
    ConvertSpecularToParams(texSpecular.Sample(sampSpecular, UV(2)), METALNESS_CHANNEL_NONE, params);
#else
    ConvertPBRFactorToParams(PBRFactor, params);
#endif

    params.NormalMap = texNormal.Sample(sampNormal, UV(3)).xy;
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input)
{
    float3 viewNormal = normalize(mul(float4(params.Normal, 0), g_MtxView).xyz);
    float3 emission = SrgbToLinear(texEmission.Sample(sampEmission, viewNormal.xy * float2(0.5, -0.5) + 0.5).rgb) * input.Color.xyz;

    params.Emission += emission * (1 - Blend.x) * GetToneMapLuminance();
}

#include "../DefaultPS.hlsli"