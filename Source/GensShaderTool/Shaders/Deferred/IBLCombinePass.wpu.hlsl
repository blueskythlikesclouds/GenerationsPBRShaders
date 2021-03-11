#include "Deferred.hlsl"
#include "IBL.hlsl"
#include "../Functions.hlsl"

float4 main(float2 vPos : TEXCOORD0, float2 texCoord : TEXCOORD1) : COLOR
{
    float depth = tex2Dlod(g_DepthSampler, float4(texCoord, 0, 0)).x;
    float3 viewPosition = GetPositionFromDepth(vPos, depth, g_MtxInvProjection);
    float3 position = mul(float4(viewPosition, 1), g_MtxInvView).xyz;

    float2 lightScattering = ComputeLightScattering(position, viewPosition);

    float4 gBuffer0 = tex2Dlod(g_GBuffer0Sampler, float4(texCoord, 0, 0));
    float4 gBuffer1 = tex2Dlod(g_GBuffer1Sampler, float4(texCoord, 0, 0));
    float4 gBuffer2 = tex2Dlod(g_GBuffer2Sampler, float4(texCoord, 0, 0));
    float4 gBuffer3 = tex2Dlod(g_GBuffer3Sampler, float4(texCoord, 0, 0));

    uint type = UnpackPrimitiveType(gBuffer3.w);

    [branch] if (type == PRIMITIVE_TYPE_RAW)
        return gBuffer0;

    [branch] if (type == PRIMITIVE_TYPE_EMISSION)
        return float4(gBuffer0.rgb * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y, 1.0);

    Material material;

    material.Albedo = gBuffer1.rgb;
    material.Alpha = gBuffer0.a;
    material.Reflectance = gBuffer2.x;
    material.Roughness = max(0.01, gBuffer2.y);
    material.AmbientOcclusion = gBuffer2.z;
    material.Metalness = gBuffer2.w;

    material.Normal = normalize(gBuffer3.xyz * 2 - 1);

    material.IndirectDiffuse = 0;
    material.IndirectSpecular = 0;

    material.ViewDirection = normalize(g_EyePosition.xyz - position);
    material.CosViewDirection = saturate(dot(material.ViewDirection, material.Normal));

    material.RoughReflectionDirection = ComputeRoughReflectionDirection(material.Roughness, material.Normal, material.ViewDirection);
    material.SmoothReflectionDirection = 2 * material.CosViewDirection * material.Normal - material.ViewDirection;

    material.F0 = lerp(material.Reflectance, material.Albedo, material.Metalness);

    float4 indirectSpecular = 0;
    if (g_IsEnablePrevIBL)
        indirectSpecular = tex2Dlod(g_PrevIBLSampler, float4(texCoord.xy, 0, mrgLodParam.y * material.Roughness));

#define COMPUTE_IBL_PROBE_THIS_PASS(index) \
    COMPUTE_IBL_PROBE(index < IterationIndex, indirectSpecular, index, material.Roughness, material.RoughReflectionDirection)

    COMPUTE_IBL_PROBE_THIS_PASS(0);
    COMPUTE_IBL_PROBE_THIS_PASS(1);
    COMPUTE_IBL_PROBE_THIS_PASS(2);
    COMPUTE_IBL_PROBE_THIS_PASS(3);
    COMPUTE_IBL_PROBE_THIS_PASS(4);
    COMPUTE_IBL_PROBE_THIS_PASS(5);
    COMPUTE_IBL_PROBE_THIS_PASS(6);
    COMPUTE_IBL_PROBE_THIS_PASS(7);

    [branch] if (indirectSpecular.a < 1.0)
    {
        indirectSpecular += float4(UnpackHDR(texCUBElod(g_DefaultIBLSampler,
            float4(material.RoughReflectionDirection * float3(1, 1, -1), material.Roughness * mrgLodParam.x))), 1.0) * (1 - indirectSpecular.a);
    }

    float sggiBlendFactor = 0.0;
    float iblBlendFactor = 1.0;

    if (type == PRIMITIVE_TYPE_GI)
    {
        sggiBlendFactor = saturate(material.Roughness * g_HDRParam_SGGIParam.w + g_HDRParam_SGGIParam.z);
        iblBlendFactor = lerp(1 - sggiBlendFactor, 1, material.Metalness);
    }

    material.IndirectSpecular = indirectSpecular * iblBlendFactor;

    float3 result = gBuffer0.rgb + ComputeIndirectLighting(material, g_EnvBRDFSampler);
    return float4(result * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y, material.Alpha);
}