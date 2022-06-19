#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 Scale : packoffset(c150);
float4 Scale2 : packoffset(c151);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texSpecular : register(t1);
Texture2D<float4> texNormal : register(t2);

Texture2D<float4> texDiffuseBlend : register(t3);
Texture2D<float4> texSpecularBlend : register(t4);
Texture2D<float4> texNormalBlend : register(t5);

Texture2D<float4> texNormalDetail : register(t6);

SamplerState sampDiffuse : register(s0);
SamplerState sampSpecular : register(s1);
SamplerState sampNormal : register(s2);

SamplerState sampDiffuseBlend : register(s3);
SamplerState sampSpecularBlend : register(s4);
SamplerState sampNormalBlend : register(s5);

SamplerState sampNormalDetail : register(s6);

float GetScale(int index)
{
    if (index >= 0 && index <= 2)
        return Scale[index];

    if (index >= 3 && index <= 6)
        return Scale2[index - 3];

    return 0.0;
}

float4 SampleTex(in PixelDeclaration input, Texture2D<float4> t, SamplerState s, int index)
{
    float scale = GetScale(index);

    float4 u = t.Sample(s, scale == 0.0 ? UV(index) : input.Position.yz * scale);
    float4 v = t.Sample(s, scale == 0.0 ? UV(index) : input.Position.xz * scale);
    float4 w = t.Sample(s, scale == 0.0 ? UV(index) : input.Position.xy * scale);

    float3 blend = normalize(input.Normal);
    blend *= blend;

    return blend.x * u + blend.y * v + blend.z * w;
}

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    float4 diffuse = lerp(
        SrgbToLinear(SampleTex(input, texDiffuse, sampDiffuse, 0)),
        SrgbToLinear(SampleTex(input, texDiffuseBlend, sampDiffuseBlend, 3)), input.Color.w);

    params.Albedo = diffuse.rgb * input.Color.rgb;
    params.Alpha = diffuse.a;

    float4 specular = SampleTex(input, texSpecular, sampSpecular, 1);

#ifdef HasSamplerSpecularBlend
    specular = lerp(specular, SampleTex(input, texSpecularBlend, sampSpecularBlend, 4), input.Color.w);
#endif

    ConvertSpecularToParams(specular, METALNESS_CHANNEL_NONE, params);

    float3 normalMap = DecodeNormalMap(SampleTex(input, texNormal, sampNormal, 2).xy);

#ifdef HasSamplerNormalBlend
    float3 normalMapBlend = DecodeNormalMap(SampleTex(input, texNormalBlend, sampNormalBlend, 5).xy);

#ifdef HasSamplerNormalDetail
    float3 normalMapDetail = DecodeNormalMap(SampleTex(input, texNormalDetail, sampNormalDetail, 6).xy);
    normalMapBlend = BlendNormalMap(normalMapBlend, normalMapDetail);
#endif

    normalMap = normalize(lerp(normalMap, normalMapBlend, input.Color.w));
#endif

    params.NormalMap = EncodeNormalMap(normalMap);
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input) {}

#include "../DefaultPS.hlsli"