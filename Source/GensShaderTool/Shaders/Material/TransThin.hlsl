#ifndef SCENE_MATERIAL_TRANS_THIN_HLSL_INCLUDED
#define SCENE_MATERIAL_TRANS_THIN_HLSL_INCLUDED

#include "../../global.psparam.hlsl"

#include "../Declarations.hlsl"
#include "../Functions.hlsl"
#include "../Material.hlsl"

#include "Default.hlsl"

sampler2D diffuseSampler : register(s0);

float4 PBRFactor : register(c150);

float4 GetDiffuse(DECLARATION_TYPE input, Material material)
{
    return float4(input.Color.rgb, tex2D(diffuseSampler, UV(0)).a);
}

float4 GetSpecular(DECLARATION_TYPE input)
{
    return float4(PBRFactor.xy, input.Color.a, 1);
}

float3 GetTrans(DECLARATION_TYPE input)
{
    return float3(0.505, 1.0, 0.12);
}

void PostProcessMaterial(DECLARATION_TYPE input, inout Material material)
{
    // Discard based on surface normal
    /*
    float3 normal = normalize(cross(ddx(input.Position), ddy(input.Position)));
    material.Alpha *= saturate(abs(dot(material.ViewDirection, normal)));
    */
}

void PostProcessFinalColor(DECLARATION_TYPE input, Material material, bool isDeferred, inout float4 finalColor)
{
}

#endif