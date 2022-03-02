#ifndef IBL_PROBE_HLSLI_INCLUDED
#define IBL_PROBE_HLSLI_INCLUDED

#ifndef GLOBALS_PS_HLSLI_INCLUDED
#include "GlobalsPS.hlsli"
#endif

#include "Shared.hlsli"

float4 ComputeIndirectIBLProbe(in ShaderParams params, float3 position, int index)
{
    float3 localPos = mul(g_IBL.Matrices[index], float4(position, 1)).xyz;

    float maxLocalPos = max(abs(localPos.x), max(abs(localPos.y), abs(localPos.z)));
    if (maxLocalPos >= 0.95)
        return 0;

    float3 localDir = mul(g_IBL.Matrices[index], float4(params.RoughReflectionDirection, 0)).xyz;

    float3 unitary = 1.0f;
    float3 firstPlaneIntersect = (unitary - localPos) / localDir;
    float3 secondPlaneIntersect = (-unitary - localPos) / localDir;

    float3 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
    float distance = min(furthestPlane.x, min(furthestPlane.y, furthestPlane.z));

    float3 intersectPosition = position.xyz + params.RoughReflectionDirection * distance;
    float3 reflectionDirection = intersectPosition - g_IBL.Params[index].xyz;

    float4 result = g_IBLProbeTextures.SampleLevel(g_LinearClampSampler, float4(reflectionDirection * float3(1, 1, -1), index), params.Roughness * g_IBL.LodParams[index]);
    return float4(result.rgb, 1) * result.a * (1 - maxLocalPos);
}

void ComputeIndirectIBLProbes(inout ShaderParams params, float3 position, float blendFactor)
{
    float4 color = 0;

    for (int i = 0; i < 24; i++)
    {
        if (color.w > 0.99)
            break;

        color += ComputeIndirectIBLProbe(params, position, i) * (1 - color.a);
    }

    if (color.w < 0.99)
        color += float4(UnpackHDR(g_DefaultIBLTexture.SampleLevel(g_LinearClampSampler, params.RoughReflectionDirection, params.Roughness * g_IBL.LodParam)), 1) * (1 - color.a);

    params.IndirectSpecular += color.rgb * blendFactor;
}

#endif