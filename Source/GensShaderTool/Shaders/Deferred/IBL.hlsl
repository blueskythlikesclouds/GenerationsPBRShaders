#ifndef SCENE_DEFERRED_IBL_INCLUDED
#define SCENE_DEFERRED_IBL_INCLUDED

#include "../../global.psparam.hlsl"
#include "../Material.hlsl"
#include "../Param.hlsl"

float3x4 mrgProbeMatrices[8] : register(c111);
float4 mrgProbeParams[8] : register(c135);
float4 mrgProbeLodParams[2] : register(c143);
float4 mrgLodParam : register(c145);

samplerCUBE g_IBLProbeSamplers[8] : register(s4);
sampler2D g_PrevIBLSampler : register(s13);
samplerCUBE g_DefaultIBLSampler : register(s14);
sampler2D g_EnvBRDFSampler : register(s15);

bool g_IsEnablePrevIBL : register(b7);

float4 ComputeIndirectIBLProbe(float3 position, float roughness, float3 reflectionDirection,
    samplerCUBE iblProbeTex, float3x4 iblProbeMatrix, float3 iblProbePosition, float iblProbeBias, float iblProbeLod)
{
    float3 localPos = mul(iblProbeMatrix, float4(position, 1)).xyz;

    float maxLocalPos = max(abs(localPos.x), max(abs(localPos.y), abs(localPos.z)));
    if (maxLocalPos >= 0.95)
        return 0;

    float3 localDir = mul(iblProbeMatrix, float4(reflectionDirection, 0)).xyz;

    float3 unitary = 1.0f;
    float3 firstPlaneIntersect = (unitary - localPos) / localDir;
    float3 secondPlaneIntersect = (-unitary - localPos) / localDir;

    float3 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
    float distance = min(furthestPlane.x, min(furthestPlane.y, furthestPlane.z));

    float3 intersectPosition = position.xyz + reflectionDirection * distance;
    reflectionDirection = intersectPosition - iblProbePosition.xyz;

    float4 result = texCUBElod(iblProbeTex, float4(reflectionDirection * float3(1, 1, -1), roughness * iblProbeLod));
    return float4(result.rgb, 1) * result.a * saturate(ComputeIndirectIBLFade(roughness) * (1 - maxLocalPos));
}

#define COMPUTE_IBL_PROBE(condition, destination, index, roughness, reflectionDirection) \
    [branch] if (condition && destination.a < 1) \
    { \
        float4 result = ComputeIndirectIBLProbe(position, roughness, reflectionDirection, \
            g_IBLProbeSamplers[index], mrgProbeMatrices[index], mrgProbeParams[index].xyz, mrgProbeParams[index].w, mrgProbeLodParams[index / 4][index % 4]); \
        \
        destination += result * (1 - destination.a); \
    }

#endif