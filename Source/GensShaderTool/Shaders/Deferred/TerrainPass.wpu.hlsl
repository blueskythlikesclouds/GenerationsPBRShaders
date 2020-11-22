#include "Deferred.hlsl"
#include "../Functions.hlsl"

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

    material.Shadow = gBuffer1.w * ComputeShadow(g_ShadowMapSampler, mul(float4(position, 1), g_MtxLightViewProjection), g_ShadowMapSize.xy);

    material.ViewDirection = normalize(g_EyePosition.xyz - position);
    material.CosViewDirection = saturate(dot(material.ViewDirection, material.Normal));

    material.ReflectionDirection = 2 * material.CosViewDirection * material.Normal - material.ViewDirection;
    material.CosReflectionDirection = saturate(dot(material.ReflectionDirection, material.Normal));

    material.F0 = lerp(material.FresnelFactor, material.Albedo, material.Metalness);

    float3 result = gBuffer0.rgb + ComputeDirectLighting(material, -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb) * material.Shadow;

    for (int i = 0; i < 32; i++)
    {
        float4 item0 = g_LocalLightData[i * 2 + 0];
        float4 item1 = g_LocalLightData[i * 2 + 1];

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
            result += ComputeDirectLighting(material, direction, lightColor) / max(0.001, attenuation);
    }

    return float4(result, material.Alpha);
}