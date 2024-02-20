#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 FalloffFactor : packoffset(c150);
float4 FurParam : packoffset(c151);
float4 AnisoFactor : packoffset(c152);
float4 FurParam2 : packoffset(c153);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D texDiffuse : register(t0);
Texture2D texFalloff : register(t1);
Texture2D texSpecular : register(t2);
Texture2D texFur : register(t3);
Texture2D texFlow : register(t4);
Texture2D texNormal : register(t5);

SamplerState sampDiffuse : register(s0);
SamplerState sampFalloff : register(s1);
SamplerState sampSpecular : register(s2);
SamplerState sampFur : register(s3);
SamplerState sampFlow : register(s4);
SamplerState sampNormal : register(s5);

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    float4 diffuse = texDiffuse.Sample(sampDiffuse, UV(0));

    params.Albedo = SrgbToLinear(diffuse.rgb);
    params.Alpha = diffuse.a;

    ConvertSpecularToParams(texSpecular.Sample(sampSpecular, UV(2)), METALNESS_CHANNEL_BLUE, params);

    params.NormalMap = texNormal.Sample(sampNormal, UV(5)).xy;
}

float4 ApplyFurParamTransform(float4 value, float furParam)
{
    value = value * 2.0 - 1.0;
    value = exp(-value * furParam);
    value = (1.0 - value) / (1.0 + value);
    float num = exp(-furParam);
    value *= 1.0 + num;
    value /= 1.0 - num;
    return value * 0.5 + 0.5;
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input)
{
    float3 falloff = SrgbToLinear(texFalloff.Sample(sampFalloff, UV(1)).rgb);
    falloff *= ComputeFalloff(params.CosViewDirection, FalloffFactor.xyz);
    params.Albedo += falloff;

    float2 flow = texFlow.Sample(sampFlow, UV(4)).xy;
    flow += 1.0 / 510.0;
    flow = flow * 2.0 - 1.0;
    flow *= 0.01 * FurParam.x;

    float2 texCoord = UV(3);
    float3 furColor = 1.0;
    float furAlpha = 0.0;

    for (uint i = 0; i < (uint) FurParam.z; i++)
    {
        float4 fur = texFur.Sample(sampFur, texCoord * FurParam.y);

        float factor = (float) i / FurParam.z;
        fur.rgb *= (1.0 - FurParam2.z) * factor + FurParam2.z;
        fur.rgb *= fur.w * FurParam2.y;

        furColor *= 1.0 - fur.w * FurParam2.y;
        furColor += fur.rgb;
        furAlpha += fur.w;

        texCoord += flow;
    }

    furColor = ApplyFurParamTransform(float4(furColor, 0.0), FurParam.w).rgb;
    furAlpha = ApplyFurParamTransform(float4(furAlpha / FurParam.z, 0.0, 0.0, 0.0), FurParam2.w).x;

    params.Albedo *= furColor;
    params.Reflectance *= furAlpha;
    params.Roughness *= max(0.01, 1.0 - (0.01 + 0.99 * furAlpha));
}

#include "../DefaultPS.hlsli"