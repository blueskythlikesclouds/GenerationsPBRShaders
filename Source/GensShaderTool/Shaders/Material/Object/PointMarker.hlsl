#ifndef SCENE_MATERIAL_OBJECT_POINT_MARKER_HLSL_INCLUDED
#define SCENE_MATERIAL_OBJECT_POINT_MARKER_HLSL_INCLUDED

#include "../../../global.psparam.hlsl"

#include "../../Declarations.hlsl"
#include "../../Functions.hlsl"
#include "../../Material.hlsl"

#include "../Default.hlsl"

float4 PBRFactor : register(c150);
sampler2D specularSampler : register(s0);

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    return saturate(float4(g_Ambient.rg * 0.5, g_Ambient.b, 1));
}

float4 GetSpecular(DECLARATION_TYPE input)
{
    return float4(PBRFactor.xy, 1, 1);
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
}

#endif