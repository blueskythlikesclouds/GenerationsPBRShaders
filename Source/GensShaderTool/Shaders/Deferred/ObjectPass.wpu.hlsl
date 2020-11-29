#include "Deferred.hlsl"
#include "../Functions.hlsl"

float3x4 mrgSHLightFieldMatrices[4] : register(c173);
float4 mrgSHLightFieldParams[4] : register(c185);

sampler3D g_SHLightFieldSamplers[4] : register(s4);

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
    material.FresnelFactor = gBuffer2.x;
    material.Roughness = max(0.01, gBuffer2.y);
    material.AmbientOcclusion = gBuffer2.z;
    material.Metalness = gBuffer2.w;

    material.Normal = normalize(gBuffer3.xyz * 2 - 1);

    material.IndirectDiffuse = 0;
    material.IndirectSpecular = 0;

    material.Shadow = ComputeShadow(g_ShadowMapSampler, mul(float4(position, 1), g_MtxLightViewProjection), g_ShadowMapParams.xy, g_ShadowMapParams.z);

    material.ViewDirection = normalize(g_EyePosition.xyz - position);
    material.CosViewDirection = saturate(dot(material.ViewDirection, material.Normal));

    material.ReflectionDirection = 2 * material.CosViewDirection * material.Normal - material.ViewDirection;
    material.CosReflectionDirection = saturate(dot(material.ReflectionDirection, material.Normal));

    material.F0 = lerp(material.FresnelFactor, material.Albedo, material.Metalness);

    float currentLength = -1;
    uint currentIndex = 0;
    float3 currentShCoords;

    uint i;

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

    switch (currentIndex)
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

    material.IndirectDiffuse = ComputeSHFinal(material.IndirectDiffuse) * 2;

    float3 direct = ComputeDirectLightingRaw(material, -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb);
    direct *= lerp(saturate(dot(-mrgGlobalLight_Direction.xyz, material.Normal)), gBuffer0.rgb, gBuffer1.w);
    direct *= material.Shadow;

    float3 indirect = ComputeIndirectLighting(material);
    indirect += lerp(gBuffer0.rgb, 0, gBuffer1.w);

    /*
    for (int i = 0; i < 32; i++)
    {
        float4 item0 = mrgLocalLightData[i * 2 + 0];
        float4 item1 = mrgLocalLightData[i * 2 + 1];

        float3 lightPosition = item0.xyz;
        float3 lightColor = item1.xyz;

        float innerRange = item0.w;
        float outerRange = item1.w;

        float3 delta = lightPosition - position;
        float distance = length(delta);
        float3 direction = delta / distance;

        float attenuation = innerRange + outerRange * distance + outerRange * distance * distance;
        attenuation -= 0.002;

        if (attenuation > 0)
            direct += ComputeDirectLighting(material, direction, lightColor) / max(0.001, attenuation);
    }
    */

    return float4(direct + indirect, material.Alpha);
}