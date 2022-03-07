#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 PBRFactor : packoffset(c150);

GLOBALS_PS_APPEND_PARAMETERS_END

void LoadParams(inout ShaderParams params, in PixelDeclaration input)
{
    params.Albedo = saturate(float3(g_Ambient.rg * 0.5, g_Ambient.b));
    ConvertPBRFactorToParams(PBRFactor, params);
}

void ModifyParams(inout ShaderParams params, in PixelDeclaration input) {}

#include "../DefaultPS.hlsli"