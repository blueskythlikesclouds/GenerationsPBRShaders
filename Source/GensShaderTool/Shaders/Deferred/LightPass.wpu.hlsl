#include "Deferred.hlsl"
#include "../Functions.hlsl"

float4 mrgLocalLightData[72] : register(c107);
float3x4 mrgSHLightFieldMatrices[3] : register(c179);
float4 mrgSHLightFieldParams[3] : register(c188);
float4 g_SSAOSize : register(c191);

sampler3D g_SHLightFieldSamplers[3] : register(s4);
sampler g_ShadowMapNoTerrainSampler : register(s7);
sampler g_SSAOSampler : register(s8);

bool g_IsEnableSSAO : register(b8);

float GetLocalLightData(int index)
{
    return mrgLocalLightData[index / 4][index % 4];
}

void ComputeSHLightField(inout Material material, in float3 position)
{
    float currentLength = -1;
    int currentIndex = 0;
    float3 currentShCoords;

    int i;

    for (i = 0; i < 3; i++)
    {
        float3 shCoords = mul(mrgSHLightFieldMatrices[i], float4(position * 10.0f, 1)).xyz;

        if (IsInSHCoordRange(shCoords))
        {
            currentIndex = i;
            currentShCoords = shCoords;
            break;
        }

        float length = dot(shCoords, shCoords);

        if (i == 0 || length < currentLength)
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
    }

    [unroll] for (i = 0; i < 6; i++)
        material.IndirectDiffuse += ComputeSHBasis(material, shlf[i], directions[i]);

    material.IndirectDiffuse = ComputeSHFinal(material.IndirectDiffuse) * g_GI0Scale.rgb;
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
        ambientOcclusionEx = tex2Dlod(g_SSAOSampler, float4(texCoord.xy, 0, 0)).x;
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

    material.RoughReflectionDirection = ComputeRoughReflectionDirection(material.Roughness, material.Normal, material.ViewDirection);
    material.SmoothReflectionDirection = 2 * material.CosViewDirection * material.Normal - material.ViewDirection;

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
        float3 lightPosition = float3(
            GetLocalLightData(i * 9 + 0),
            GetLocalLightData(i * 9 + 1),
            GetLocalLightData(i * 9 + 2)
        );

        float3 lightColor = float3(
            GetLocalLightData(i * 9 + 3),
            GetLocalLightData(i * 9 + 4),
            GetLocalLightData(i * 9 + 5)
        );

        float4 lightRange = float4(
            0,
            GetLocalLightData(i * 9 + 6),
            GetLocalLightData(i * 9 + 7),
            GetLocalLightData(i * 9 + 8)
        );
        
        direct += ComputeLocalLight(position, material, lightPosition, lightColor, lightRange);
    } 

    return float4(direct + indirect, material.Alpha);
}