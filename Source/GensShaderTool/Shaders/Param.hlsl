#ifndef PARAM_HLSL_INCLUDED
#define PARAM_HLSL_INCLUDED

// Raw: GBuffer0 has raw color. Nothing is applied.
// Emission: GBuffer0 has emission color. Light scattering is applied.
// GI: GBuffer0 has GI and emission color. Direct light, local light, IBL and light scattering is applied.
// No GI: GBuffer0 has emission color. Direct light, local light, SHLF, IBL and light scattering is applied.
// CDR: GBuffer0 has CDR color. Direct light, local light, SHLF, IBL and light scattering is applied.

#define PRIMITIVE_TYPE_RAW          0
#define PRIMITIVE_TYPE_EMISSION     1
#define PRIMITIVE_TYPE_GI           2
#define PRIMITIVE_TYPE_NO_GI        3
#define PRIMITIVE_TYPE_CDR          4
#define PRIMITIVE_TYPE_MAX          4

float4 g_GIParam : register(c106);
float4 g_SGGIParam : register(c107);
float4 g_MiddleGray_Scale_LuminanceLow_LuminanceHigh : register(c108);

float4 g_DebugParam[2] : register(c222);

#if defined(GLOBAL_PSPARAM_HLSL_INCLUDED) || defined(GLOBAL_VSPARAM_HLSL_INCLUDED)

#include "Functions.hlsl"

float2 ComputeLightScattering(float3 position, float3 viewPosition)
{
    float4 r0, r3, r4;

    r0.x = -viewPosition.z + -g_LightScatteringFarNearScale.y;
    r0.x = saturate(r0.x * g_LightScatteringFarNearScale.x);
    r0.x = r0.x * g_LightScatteringFarNearScale.z;
    r0.y = g_LightScattering_Ray_Mie_Ray2_Mie2.y + g_LightScattering_Ray_Mie_Ray2_Mie2.x;
    r0.z = rcp(r0.y);
    r0.x = r0.x * -r0.y;
    r0.x = r0.x * LOG2E;
    r0.x = exp2(r0.x);
    r0.y = -r0.x + 1;
    r3.xyz = -position + g_EyePosition.xyz;
    r4.xyz = normalize(r3.xyz);
    r3.x = dot(-mrgGlobalLight_Direction.xyz, r4.xyz);
    r3.y = g_LightScattering_ConstG_FogDensity.z * r3.x + g_LightScattering_ConstG_FogDensity.y;
    r4.x = pow(abs(r3.y), 1.5);
    r3.y = rcp(r4.x);
    r3.y = r3.y * g_LightScattering_ConstG_FogDensity.x;
    r3.y = r3.y * g_LightScattering_Ray_Mie_Ray2_Mie2.w;
    r3.x = r3.x * r3.x + 1;
    r3.x = g_LightScattering_Ray_Mie_Ray2_Mie2.z * r3.x + r3.y;
    r0.z = r0.z * r3.x;
    r0.y = r0.y * r0.z;

    return float2(r0.x, r0.y * g_LightScatteringFarNearScale.w);
}

#endif

float PackPrimitiveType(uint primitiveType)
{
    return primitiveType / (float)PRIMITIVE_TYPE_MAX;
}

uint UnpackPrimitiveType(float primitiveType)
{
    return uint(abs(round(primitiveType * PRIMITIVE_TYPE_MAX)));
}

#endif