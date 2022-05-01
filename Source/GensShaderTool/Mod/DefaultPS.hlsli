#ifndef DEFAULT_PS_HLSLI_INCLUDED
#define DEFAULT_PS_HLSLI_INCLUDED

#include "SHLightField.hlsli"
#include "IBLProbe.hlsli"
#include "SharedPS.hlsli"

cbuffer cbGITexture : register(b4)
{
    float4 g_GIAtlasParam;
    float4 g_OcclusionAtlasParam;
    bool g_IsSG;
    bool g_HasOcclusion;
}

void main(in PixelDeclaration input,

#ifdef IsPermutationDeferred
    out float4 gBuffer0 : SV_TARGET0,
    out float4 gBuffer1 : SV_TARGET1,
    out float4 gBuffer2 : SV_TARGET2,
    out float4 gBuffer3 : SV_TARGET3
#else
    out float4 outColor : SV_TARGET0
#endif
    )
{
    ShaderParams params = CreateShaderParams();
    LoadParams(params, input);

    if (g_Flags & SHARED_FLAGS_ENABLE_ALPHA_TEST)
        clip(params.Alpha - g_AlphaThreshold);

    params.Normal = normalize(input.Normal);

#ifdef HasFeatureNormalMapping

    if (!g_UseFlatNormal)
    {
        float3 normalMap = DecodeNormalMap(params.NormalMap);

        params.Normal = normalize(
            normalMap.x * normalize(input.Tangent) +
            normalMap.y * normalize(input.Binormal) +
            normalMap.z * normalize(input.Normal));
    }

#endif

    ComputeShadingParams(params, input.Position);
    ModifyParams(params, input);

    if (g_UseWhiteAlbedo) params.Albedo = 1.0;
    if (g_ReflectanceOverride >= 0) params.Reflectance = g_ReflectanceOverride;
    if (g_RoughnessOverride >= 0) params.Roughness = g_RoughnessOverride;
    if (g_AmbientOcclusionOverride >= 0) params.AmbientOcclusion = g_AmbientOcclusionOverride;
    if (g_MetalnessOverride >= 0) params.Metalness = g_MetalnessOverride;

#ifdef NoGI
    float sggiBlendFactor = 0.0;
    float iblBlendFactor = 1.0;

#ifndef IsPermutationDeferred
    ComputeSHLightField(params, input.Position);
#endif

#else
    float sggiBlendFactor = saturate(g_SGGIParam.x + params.Roughness * g_SGGIParam.y);
    float iblBlendFactor = lerp(1.0 - sggiBlendFactor, 1.0, params.Metalness);

    const float2 giCoord = input.TexCoord0.zw * g_GIAtlasParam.xy + g_GIAtlasParam.zw;
    const float2 occlusionCoord = input.TexCoord0.zw * g_OcclusionAtlasParam.xy + g_OcclusionAtlasParam.zw;

    if (g_IsSG)
    {
        float3 sggi[4];

        int i;

        for (i = 0; i < 4; i++)
            sggi[i] = g_SGGITexture.Sample(g_LinearClampSampler, float3(giCoord, i));

        params.Shadow = g_OcclusionTexture.Sample(g_LinearClampSampler, occlusionCoord);

        const float3 axis[4] =
        {
            float3(0, 0.57735002, 1),
            float3(0, 0.57735002, -1),
            float3(1, 0.57735002, 0),
            float3(-1, 0.57735002, 0)
        };

        params.IndirectDiffuse = 0;

        for (i = 0; i < 4; i++)
            params.IndirectDiffuse += ComputeSggiDiffuse(params, sggi[i], axis[i]);

        params.IndirectDiffuse *= ComputeSggiDiffuseFactor();

        float4 r6;
        float sggiSpecularFactor = ComputeSggiSpecularFactor(params, r6);

        params.IndirectSpecular = 0;

        for (i = 0; i < 4; i++)
            params.IndirectSpecular += ComputeSggiSpecular(params, sggi[i], axis[i], r6);

        params.IndirectSpecular *= sggiSpecularFactor;
        params.IndirectSpecular *= sggiBlendFactor;
    }
    else
    {
        float4 gi = g_GITexture.Sample(g_LinearClampSampler, giCoord);

        if (g_HasOcclusion)
        {
            params.IndirectDiffuse = gi.rgb;
            params.Shadow = g_OcclusionTexture.Sample(g_LinearClampSampler, occlusionCoord);
        }

        else
        {
            params.IndirectDiffuse = gi.rgb * gi.rgb;
            params.Shadow = gi.a;
        }
    }

    if (g_GIColorOverride.r >= 0)
    {
        params.IndirectDiffuse = g_GIColorOverride;
        params.IndirectSpecular = 0;
    }
    if (g_GIShadowOverride >= 0) params.Shadow = g_GIShadowOverride;

    params.IndirectDiffuse *= g_GI0Scale.rgb;
    params.IndirectSpecular *= g_GI0Scale.rgb;
#endif

#ifndef IsPermutationDeferred
    params.IndirectSpecular += ComputeIndirectIBLProbes(params, input.Position) * iblBlendFactor;

    params.Shadow *= ComputeShadow(g_ShadowMapTexture, g_PointBorderSampler, g_ShadowMapSize, g_ESMFactor, mul(float4(input.Position, 1.0), g_MtxLightViewProjection));

    bool cdr = false;
#ifdef HasSamplerCdr
    cdr = true;
#endif

    outColor.rgb = ComputeDirectLighting(params, -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.xyz, cdr) * params.Shadow;
    outColor.rgb += ComputeLocalLights(params, input.Position);
    outColor.rgb += ComputeIndirectLighting(params, g_EnvBRDFTexture, g_LinearClampSampler);
    outColor.rgb = lerp(outColor.rgb, params.Refraction.rgb, params.Refraction.w);
    outColor.rgb += params.Emission;
    outColor.rgb = outColor.rgb * input.LightScattering.x + g_LightScatteringColor.rgb * input.LightScattering.y;
    outColor.a = params.Alpha;

#else
    StoreParams(params, gBuffer0, gBuffer1, gBuffer2, gBuffer3);
#endif
}

#endif