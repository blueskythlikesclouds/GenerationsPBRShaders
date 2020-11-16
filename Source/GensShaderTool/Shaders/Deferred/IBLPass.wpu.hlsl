#include "Deferred.hlsl"
#include "../Functions.hlsl"

float3x4 g_IBLProbeMatrices[8] : register(c150);
float4 g_IBLProbeParams[8] : register(c174);
float4 g_IBLProbeLODParams[2] : register(c182);

float4 g_OtherLODParam : register(c184);

samplerCUBE g_IBLProbeSamplers[8] : register(s4);
sampler2D g_RLRSampler : register(s13);
samplerCUBE g_DefaultIBLSampler : register(s14);
sampler2D g_EnvBRDFSampler : register(s15);

float4 ComputeIndirectIBLProbe(Material material, float3 position, samplerCUBE iblProbeTex, float3x4 iblProbeMatrix, float3 iblProbePosition, float iblProbeBias, float iblProbeLod)
{
    float3 localPos = mul(iblProbeMatrix, float4(position, 1)).xyz;

    if (abs(localPos.x) >= 0.95 || abs(localPos.y) >= 0.95 || abs(localPos.z) >= 0.95)
        return 0;

    float3 localDir = mul(iblProbeMatrix, float4(material.ReflectionDirection, 0)).xyz;

    float3 unitary = 1.0f;
    float3 firstPlaneIntersect = (unitary - localPos) / localDir;
    float3 secondPlaneIntersect = (-unitary - localPos) / localDir;

    float3 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
    float distance = min(furthestPlane.x, min(furthestPlane.y, furthestPlane.z));

    float3 intersectPosition = position.xyz + material.ReflectionDirection * distance;
    float3 reflectionDirection = intersectPosition - iblProbePosition.xyz;

    float4 result = texCUBElod(iblProbeTex, float4(reflectionDirection * float3(1, 1, -1), material.Roughness * iblProbeLod));
    return float4(result.rgb * result.a, result.a);
}

float4 main(float2 vPos : TEXCOORD0, float2 texCoord : TEXCOORD1) : COLOR
{
    float3 viewPosition = GetPositionFromDepth(vPos, tex2Dlod(g_DepthSampler, float4(texCoord, 0, 0)).x, g_MtxInvProjection);
    float3 position = mul(float4(viewPosition, 1), g_MtxInvView).xyz;

    float4 gBuffer0 = tex2Dlod(g_GBuffer0Sampler, float4(texCoord, 0, 0));
    float4 gBuffer1 = tex2Dlod(g_GBuffer1Sampler, float4(texCoord, 0, 0));
    float4 gBuffer2 = tex2Dlod(g_GBuffer2Sampler, float4(texCoord, 0, 0));
    float4 gBuffer3 = tex2Dlod(g_GBuffer3Sampler, float4(texCoord, 0, 0));

    Material material;

    material.Albedo = gBuffer1.rgb;
    material.Alpha = gBuffer0.a;
    material.Metalness = gBuffer2.x;
    material.Roughness = gBuffer2.y;
    material.AmbientOcclusion = gBuffer2.z;
    material.GIContribution = gBuffer2.w;

    material.Normal = normalize(gBuffer3.xyz * 2 - 1);

    material.IndirectDiffuse = 0;
    material.IndirectSpecular = 0;

    material.ViewDirection = normalize(g_EyePosition.xyz - position);
    material.CosViewDirection = saturate(dot(material.ViewDirection, material.Normal));

    material.ReflectionDirection = 2 * material.CosViewDirection * material.Normal - material.ViewDirection;
    material.CosReflectionDirection = saturate(dot(material.ReflectionDirection, material.Normal));

    material.F0 = lerp(0.04, material.Albedo, material.Metalness);

    float4 indirectSpecular = tex2Dlod(g_RLRSampler, float4(texCoord.xy, 0, g_OtherLODParam.y * material.Roughness));

    // imagine not being able to unroll loops that access samplers with indexers smh my head 

#define COMPUTE_PROBE(index, paramIndex, paramSwizzle) \
    if (indirectSpecular.a < 1.0) \
    { \
        indirectSpecular += ComputeIndirectIBLProbe(material, position, \
            g_IBLProbeSamplers[index], g_IBLProbeMatrices[index], g_IBLProbeParams[index].xyz, g_IBLProbeParams[index].w, g_IBLProbeLODParams[paramIndex].paramSwizzle) * (1 - indirectSpecular.a); \
    }

    COMPUTE_PROBE(0, 0, x)
    COMPUTE_PROBE(1, 0, y)
    COMPUTE_PROBE(2, 0, z)
    COMPUTE_PROBE(3, 0, w)
    COMPUTE_PROBE(4, 1, x)
    COMPUTE_PROBE(5, 1, y)
    COMPUTE_PROBE(6, 1, z)
    COMPUTE_PROBE(7, 1, w)

#undef COMPUTE_PROBE

    if (indirectSpecular.a < 1.0)
        indirectSpecular += float4(UnpackHDR(texCUBElod(g_DefaultIBLSampler, float4(material.ReflectionDirection * float3(1, 1, -1), material.Roughness * g_OtherLODParam.x))), 1.0) * (1 - indirectSpecular.a);

    material.IndirectSpecular = indirectSpecular;
    material.IndirectSpecular *= 1 - saturate((material.Roughness - 0.3) * 2.85714);

    return float4(gBuffer0.rgb + ComputeIndirectLighting(material, g_EnvBRDFSampler), material.Alpha);
}