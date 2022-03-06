#ifndef IBL_PROBE_HLSLI_INCLUDED
#define IBL_PROBE_HLSLI_INCLUDED

#ifndef GLOBALS_PS_HLSLI_INCLUDED
#include "GlobalsPS.hlsli"
#endif

#include "Shared.hlsli"

float4 ComputeIndirectIBLProbe(in ShaderParams params, float3 position, uint index)
{
    float3 localPos = mul(mrgIBLProbeMatrices[index], float4(position, 1)).xyz;

    float maxLocalPos = max(abs(localPos.x), max(abs(localPos.y), abs(localPos.z)));
    if (maxLocalPos >= 0.95)
        return 0;

    float3 localDir = mul(mrgIBLProbeMatrices[index], float4(params.RoughReflectionDirection, 0)).xyz;

    float3 unitary = 1.0f;
    float3 firstPlaneIntersect = (unitary - localPos) / localDir;
    float3 secondPlaneIntersect = (-unitary - localPos) / localDir;

    float3 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
    float distance = min(furthestPlane.x, min(furthestPlane.y, furthestPlane.z));

    float3 intersectPosition = position.xyz + params.RoughReflectionDirection * distance;
    float3 reflectionDirection = intersectPosition - mrgIBLProbeParams[index].xyz;

    float4 result = g_IBLProbeTextures.SampleLevel(g_LinearClampSampler, 
        float4(reflectionDirection * float3(1, 1, -1), mrgIBLProbeParams[index].w), params.Roughness * mrgIBLProbeLodParam);

    return float4(result.rgb, 1.0) * result.a * saturate((1 - maxLocalPos) / min(0.5, params.Roughness));
}

float3 ComputeIndirectIBLProbes(inout ShaderParams params, float3 position)
{
    float4 color = 0;

    for (uint i = 0; i < mrgIBLProbeCount; i++)
    {
        if (color.w > 0.99)
            break;

        color += ComputeIndirectIBLProbe(params, position, i) * (1 - color.a);
    }

    if (color.w < 0.99)
    {
        color += float4(UnpackHDR(g_DefaultIBLTexture.SampleLevel(g_LinearClampSampler, 
            params.RoughReflectionDirection, params.Roughness * mrgDefaultIBLLodParam)), 1) * (1 - color.a);
    }

    return color.rgb;
}

#endif