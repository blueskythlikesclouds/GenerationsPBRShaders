#ifndef SCENE_MATERIAL_DEFAULT_HLSL_INCLUDED
#define SCENE_MATERIAL_DEFAULT_HLSL_INCLUDED

#include "../Declarations.hlsl"

#if (defined(IsChrEyeCDRF) && IsChrEyeCDRF) || \
    (defined(IsEye2) && IsEye2)

#define DECLARATION_TYPE    EyeDeclaration

#elif (defined(HasNormal) && HasNormal) || \
      (defined(IsDefault2Normal) && IsDefault2Normal)

#define DECLARATION_TYPE    NormalDeclaration

#elif defined(IsWater2) && IsWater2

#define DECLARATION_TYPE    WaterDeclaration

#else
#define DECLARATION_TYPE    DefaultDeclaration
#endif

float4 g_GIParam : register(c189);
float4 g_SGGIParam : register(c190);

float4 g_GIAtlasSize : register(c191);
float4 g_ShadowMapSize : register(c192);
float4 g_MiddleGray_Scale_LuminanceLow_LuminanceHigh : register(c193);

bool g_IsUseDeferred : register(b6);
bool g_IsUseSGGI : register(b7);
bool g_IsUseCubicFilter : register(b8);

samplerCUBE g_DefaultIBLSampler : register(s14);
sampler2D g_EnvBRDFSampler : register(s15);

#endif