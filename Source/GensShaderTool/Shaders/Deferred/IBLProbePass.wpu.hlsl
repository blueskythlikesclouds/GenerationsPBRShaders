#include "Deferred.hlsl"
#include "IBL.hlsl"
#include "../Functions.hlsl"

float4 main(float2 vPos : TEXCOORD0, float2 texCoord : TEXCOORD1) : COLOR
{
    float4 gBuffer2 = tex2Dlod(g_GBuffer2Sampler, float4(texCoord, 0, 0));

    float4 indirectSpecular = 0;

    if (g_IsEnablePrevIBL)
        indirectSpecular = tex2Dlod(g_PrevIBLSampler, float4(texCoord, 0, gBuffer2.y * mrgLodParam.y));

    [branch] if (indirectSpecular.a > 0.99)
        return indirectSpecular;

    float4 gBuffer3 = tex2Dlod(g_GBuffer3Sampler, float4(texCoord, 0, 0));
    uint type = UnpackPrimitiveType(gBuffer3.w);
    
    [branch] if (type == PRIMITIVE_TYPE_RAW || type == PRIMITIVE_TYPE_EMISSION)
        return indirectSpecular;

    float depth = tex2Dlod(g_DepthSampler, float4(texCoord, 0, 0)).x;
    float3 viewPosition = GetPositionFromDepth(vPos, depth, g_MtxInvProjection);
    float3 position = mul(float4(viewPosition, 1), g_MtxInvView).xyz;

    float3 viewDirection = normalize(g_EyePosition.xyz - position);
    float3 reflectionDirection = ComputeRoughReflectionDirection(gBuffer2.y, normalize(gBuffer3.xyz * 2 - 1), viewDirection);

#define COMPUTE_IBL_PROBE_THIS_PASS(index) \
    COMPUTE_IBL_PROBE(true, indirectSpecular, index, gBuffer2.y, reflectionDirection)

    COMPUTE_IBL_PROBE_THIS_PASS(0);
    COMPUTE_IBL_PROBE_THIS_PASS(1);
    COMPUTE_IBL_PROBE_THIS_PASS(2);
    COMPUTE_IBL_PROBE_THIS_PASS(3);
    COMPUTE_IBL_PROBE_THIS_PASS(4);
    COMPUTE_IBL_PROBE_THIS_PASS(5);
    COMPUTE_IBL_PROBE_THIS_PASS(6);
    COMPUTE_IBL_PROBE_THIS_PASS(7);

    return indirectSpecular;
}