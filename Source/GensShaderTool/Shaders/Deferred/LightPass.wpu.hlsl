#include "Deferred.hlsl"
#include "../Functions.hlsl"

float3x4 mrgSHLightFieldMatrices[4] : register(c173);
float4 mrgSHLightFieldParams[4] : register(c185);

float4 g_SSAOSize : register(c189);

sampler3D g_SHLightFieldSamplers[4] : register(s4);

sampler g_ShadowMapNoTerrainSampler : register(s9);

sampler g_SSAOSampler : register(s10);

bool g_IsEnableSSAO : register(b8);

void ComputeSHLightField(inout Material material, in float3 position)
{
    float currentLength = -1;
    int currentIndex = 0;
    float3 currentShCoords;

    int i;

    for (i = 0; i < 4; i++)
    {
        float3 shCoords = mul(mrgSHLightFieldMatrices[i], float4(position, 1)).xyz;

        if (IsInSHCoordRange(shCoords))
        {
            currentIndex = i;
            currentShCoords = shCoords;
            break;
        }

        float length = dot(shCoords, shCoords);

        if (length < currentLength)
        {
            currentLength = length;
            currentIndex = i;
            currentShCoords = shCoords;
        }
    }

    const float3 directions[6] =
    {
        float3(1, 0, 0),
        float3(-1, 0, 0),
        float3(0, 1, 0),
        float3(0, -1, 0),
        float3(0, 0, 1),
        float3(0, 0, -1)
    };

    float3 shlf[6];

    switch (abs(currentIndex))
    {
    case 0:
    default:
        currentShCoords = ComputeSHTexCoords(currentShCoords, mrgSHLightFieldParams[0]);

        for (i = 0; i < 6; i++)
            shlf[i] = tex3Dlod(g_SHLightFieldSamplers[0], float4(currentShCoords + float3(i / 9.0f, 0, 0), 0)).rgb;

        break;

    case 1:
        currentShCoords = ComputeSHTexCoords(currentShCoords, mrgSHLightFieldParams[1]);

        for (i = 0; i < 6; i++)
            shlf[i] = tex3Dlod(g_SHLightFieldSamplers[1], float4(currentShCoords + float3(i / 9.0f, 0, 0), 0)).rgb;

        break;

    case 2:
        currentShCoords = ComputeSHTexCoords(currentShCoords, mrgSHLightFieldParams[2]);

        for (i = 0; i < 6; i++)
            shlf[i] = tex3Dlod(g_SHLightFieldSamplers[2], float4(currentShCoords + float3(i / 9.0f, 0, 0), 0)).rgb;

        break;

    case 3:
        currentShCoords = ComputeSHTexCoords(currentShCoords, mrgSHLightFieldParams[3]);

        for (i = 0; i < 6; i++)
            shlf[i] = tex3Dlod(g_SHLightFieldSamplers[3], float4(currentShCoords + float3(i / 9.0f, 0, 0), 0)).rgb;

        break;
    }

    [unroll] for (i = 0; i < 6; i++)
        material.IndirectDiffuse += ComputeSHBasis(material, shlf[i], directions[i]);

    material.IndirectDiffuse = ComputeSHFinal(material.IndirectDiffuse);
}

float4 main(float2 vPos : TEXCOORD0, float2 texCoord : TEXCOORD1, out float4 oGBuffer2 : COLOR1) : COLOR
{
    float3 viewPosition = GetPositionFromDepth(vPos, tex2Dlod(g_DepthSampler, float4(texCoord, 0, 0)).x, g_MtxInvProjection);
    float3 position = mul(float4(viewPosition, 1), g_MtxInvView).xyz;

    float4 gBuffer0 = tex2Dlod(g_GBuffer0Sampler, float4(texCoord, 0, 0));
    float4 gBuffer1 = tex2Dlod(g_GBuffer1Sampler, float4(texCoord, 0, 0));
    float4 gBuffer2 = tex2Dlod(g_GBuffer2Sampler, float4(texCoord, 0, 0));
    float4 gBuffer3 = tex2Dlod(g_GBuffer3Sampler, float4(texCoord, 0, 0));

    uint type = UnpackPrimitiveType(gBuffer3.w);

    if (type == PRIMITIVE_TYPE_RAW || type == PRIMITIVE_TYPE_EMISSION)
        return gBuffer0;

    float ambientOcclusionEx = 1.0;
    if (g_IsEnableSSAO)
    {
        ambientOcclusionEx = tex2DlodFastBicubic(g_SSAOSampler, texCoord.x * g_SSAOSize.x, texCoord.y * g_SSAOSize.y, g_SSAOSize.zw, 0).x;
        oGBuffer2 = float4(gBuffer2.x, gBuffer2.y, gBuffer2.z * ambientOcclusionEx, gBuffer2.w);
    }

    Material material;

    material.Albedo = gBuffer1.rgb;
    material.Alpha = gBuffer0.a;
    material.Reflectance = gBuffer2.x;
    material.Roughness = max(0.01, gBuffer2.y);
    material.AmbientOcclusion = gBuffer2.z * ambientOcclusionEx;
    material.Metalness = gBuffer2.w;

    material.Normal = normalize(gBuffer3.xyz * 2 - 1);

    material.IndirectDiffuse = 0;
    material.IndirectSpecular = 0;

    material.Shadow = gBuffer1.w;

    material.ViewDirection = normalize(g_EyePosition.xyz - position);
    material.CosViewDirection = saturate(dot(material.ViewDirection, material.Normal));

    material.ReflectionDirection = 2 * material.CosViewDirection * material.Normal - material.ViewDirection;
    material.CosReflectionDirection = saturate(dot(material.ReflectionDirection, material.Normal));

    material.F0 = lerp(material.Reflectance, material.Albedo, material.Metalness);

    float3 direct;
    float3 indirect;

    if (type == PRIMITIVE_TYPE_GI)
    {
        direct = ComputeDirectLighting(material, -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb);
        indirect = gBuffer0.rgb * ambientOcclusionEx;

        material.Shadow *= ComputeShadow(g_ShadowMapNoTerrainSampler, mul(float4(position, 1), g_MtxLightViewProjection), g_ShadowMapParams.xy, g_ShadowMapParams.z);
    }
    else
    {
        ComputeSHLightField(material, position);

        indirect = ComputeIndirectLighting(material);

        direct = ComputeDirectLightingRaw(material, -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb);

        if (type == PRIMITIVE_TYPE_CDR)
        {
            direct *= gBuffer0.rgb;
        }
        else
        {
            direct *= saturate(dot(-mrgGlobalLight_Direction.xyz, material.Normal));
            indirect += gBuffer0.rgb;
        }

        material.Shadow *= ComputeShadow(g_ShadowMapSampler, mul(float4(position, 1), g_MtxLightViewProjection), g_ShadowMapParams.xy, g_ShadowMapParams.z);
    }

    direct *= material.Shadow;

    [unroll] for (int i = 0; i < IterationIndex; i++)
    {
        float4 item0 = mrgLocalLightData[i * 2 + 0];
        float4 item1 = mrgLocalLightData[i * 2 + 1];

        float3 lightPosition = item0.xyz;
        float3 lightColor = item1.xyz;

        float innerRange = item0.w;
        float outerRange = item1.w;

        direct += ComputeLocalLight(position, material, lightPosition, lightColor, float4(0, innerRange, outerRange, outerRange));
    } 

    return float4(direct + indirect, material.Alpha);
}