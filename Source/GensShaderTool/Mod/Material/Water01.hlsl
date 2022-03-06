#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 PBRFactor : packoffset(c150);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texNormal : register(t1);
Texture2D<float4> texNormal1 : register(t2);

SamplerState sampDiffuse : register(s0);
SamplerState sampNormal : register(s1);
SamplerState sampNormal1 : register(s2);

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    float4 diffuse = texDiffuse.Sample(sampDiffuse, UV(0));

    params.Albedo = SrgbToLinear(diffuse.rgb);
    params.Alpha = diffuse.a;

    ConvertPBRFactorToParams(PBRFactor, params);

    params.NormalMap = lerp(texNormal.Sample(sampNormal, UV(1)).xy, 
        texNormal1.Sample(sampNormal1, UV(2)).xy, input.Color.w);
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input) {}

#include "../DefaultPS.hlsli"