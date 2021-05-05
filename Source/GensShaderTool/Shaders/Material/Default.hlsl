#ifndef SCENE_MATERIAL_DEFAULT_HLSL_INCLUDED
#define SCENE_MATERIAL_DEFAULT_HLSL_INCLUDED

#include "../Declarations.hlsl"
#include "../Param.hlsl"

#if (defined(IsChrEyeCDRF) && IsChrEyeCDRF) || \
    (defined(IsEye2) && IsEye2)

#define DECLARATION_TYPE    EyeDeclaration

#elif (defined(HasNormal) && HasNormal) || \
      (defined(IsDefault2Normal) && IsDefault2Normal) || \
      (defined(IsWater2) && IsWater2)

#define DECLARATION_TYPE    NormalDeclaration

#else
#define DECLARATION_TYPE    DefaultDeclaration
#endif

#ifndef GLOBAL_VSPARAM_HLSL_INCLUDED
float4 mrgGIAtlasParam : register(c107);
#endif

float4 mrgOcclusionAtlasParam : register(c108);

bool mrgIsUseDeferred : register(b8);
bool mrgIsSG : register(b9);
bool mrgHasOcclusion : register(b10);

sampler2D g_LuminanceSampler : register(s8);
sampler2D g_OcclusionSampler : register(s9);
samplerCUBE g_DefaultIBLSampler : register(s14);
sampler2D g_EnvBRDFSampler : register(s15);

float GetToneMapLuminance()
{
    return tex2Dlod(g_LuminanceSampler, 0).x * g_HDRParam_SGGIParam.x;
}

#endif