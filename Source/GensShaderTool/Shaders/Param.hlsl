#ifndef PARAM_HLSL_INCLUDED
#define PARAM_HLSL_INCLUDED

float4 g_GIParam : register(c106);
float4 g_SGGIParam : register(c107);
float4 g_MiddleGray_Scale_LuminanceLow_LuminanceHigh : register(c108);

float4 g_DebugParam[2] : register(c222);

bool g_IsUseCubicFilter : register(b6);
bool g_IsEnableInverseToneMap : register(b7);

#if defined(GLOBAL_PSPARAM_HLSL_INCLUDED) || defined(GLOBAL_VSPARAM_HLSL_INCLUDED)

#include "Functions.hlsl"

float2 ComputeLightScattering(float3 position)
{
    float4 r0, r1, r2, r3, r4, result;

    r1.xyz = g_EyePosition.xyzw + -position;
    r2.xyz = normalize(r1.xyz);
    r1.x = dot(-mrgGlobalLight_Direction.xyzw, r2.xyz);
    r1.y = g_LightScattering_ConstG_FogDensity.z * r1.x + g_LightScattering_ConstG_FogDensity.y;
    r1.x = r1.x * r1.x + 1;
    r2.x = pow(abs(r1.y), 1.5);
    r1.y = 1.0 / r2.x;
    r1.y = g_LightScattering_ConstG_FogDensity.x * r1.y;
    r1.y = g_LightScattering_Ray_Mie_Ray2_Mie2.w * r1.y;
    r1.x = g_LightScattering_Ray_Mie_Ray2_Mie2.z * r1.x + r1.y;
    r1.y = g_LightScattering_Ray_Mie_Ray2_Mie2.x + g_LightScattering_Ray_Mie_Ray2_Mie2.y;
    r1.z = 1.0 / r1.y;
    r1.x = r1.x * r1.z;
    r2 = mul(float4(position, 1), g_MtxView);
    r0.x = -g_LightScatteringFarNearScale.y + -r2.z;
    r0.x = saturate(g_LightScatteringFarNearScale.x * r0.x);
    r0.x = g_LightScatteringFarNearScale.z * r0.x;
    r0.x = -r1.y * r0.x;
    r0.x = LOG2E * r0.x;
    r0.x = exp2(r0.x);
    r0.y = 1 + -r0.x;
    result.x = r0.x;
    r0.x = r1.x * r0.y;
    result.y = g_LightScatteringFarNearScale.w * r0.x;

    return result;
}

#endif

#endif