#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 PBRFactor : packoffset(c150);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> texDiffuse : register(t0);
SamplerState sampDiffuse : register(s0);

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    params.Albedo = SrgbToLinear(input.Color.rgb);
    params.Alpha = texDiffuse.Sample(sampDiffuse, UV(0)).a;

    ConvertPBRFactorToParams(PBRFactor, params);
    params.AmbientOcclusion = input.Color.a;

    params.DeferredFlags |= DEFERRED_FLAGS_LIGHT_FIELD;
    params.DeferredFlags |= DEFERRED_FLAGS_TRANSLUCENCY;
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input) {}

#include "../DefaultPS.hlsli"