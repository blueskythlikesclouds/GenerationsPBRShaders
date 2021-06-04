#include "../global.psparam.hlsl"

#include "Filter.hlsl"
#include "Functions.hlsl"
#include "Material.hlsl"

#include "Material/Default.hlsl"

#if (defined(IsCommon2) && IsCommon2) || (defined(IsMCommon) && IsMCommon)
#include "Material/Common.hlsl"
#elif (defined(IsBlend2) && IsBlend2) || (defined(IsMBlend) && IsMBlend)
#include "Material/Blend.hlsl"
#elif (defined(IsChrEyeCDRF) && IsChrEyeCDRF) || (defined(IsChrEyeSuper) && IsChrEyeSuper)
#include "Material/Eye.hlsl"
#elif defined(IsChrSkinCDRF) && IsChrSkinCDRF
#include "Material/Skin.hlsl"
#elif defined(IsWater01) && IsWater01
#include "Material/Water01.hlsl"
#elif defined(IsWater05) && IsWater05
#include "Material/Water05.hlsl"
#elif defined(IsRing2) && IsRing2
#include "Material/Ring.hlsl"
#elif (defined(IsEmission) && IsEmission) || (defined(IsMEmission) && IsMEmission)
#include "Material/Emission.hlsl"
#elif defined(IsGlass2) && IsGlass2
#include "Material/Glass.hlsl"
#elif defined(IsChrGlass) && IsChrGlass
#include "Material/ChrGlass.hlsl"
#elif defined(IsDry) && IsDry
#include "Material/Dry.hlsl"
#elif (defined(IsFalloff2) && IsFalloff2) || (defined(IsMFalloff) && IsMFalloff)
#include "Material/Falloff.hlsl"
#elif (defined(IsChrEmission) && IsChrEmission) || (defined(IsMChrEmission) && IsMChrEmission)
#include "Material/ChrEmission.hlsl"
#elif (defined(IsPointMarker) && IsPointMarker)
#include "Material/Object/PointMarker.hlsl"
#elif (defined(IsSuperSonic) && IsSuperSonic)
#include "Material/SuperSonic.hlsl"
#endif

float3 ComputeLocalLight(in DECLARATION_TYPE input, in Material material)
{
    float3 result = 0;

#if !defined(NoLight) || !NoLight
    result += ComputeLocalLight(input.Position, material, mrgLocalLight0_Position.xyz, mrgLocalLight0_Color.rgb, mrgLocalLight0_Range);
    result += ComputeLocalLight(input.Position, material, mrgLocalLight1_Position.xyz, mrgLocalLight1_Color.rgb, mrgLocalLight1_Range);
    result += ComputeLocalLight(input.Position, material, mrgLocalLight2_Position.xyz, mrgLocalLight2_Color.rgb, mrgLocalLight2_Range);
    result += ComputeLocalLight(input.Position, material, mrgLocalLight3_Position.xyz, mrgLocalLight3_Color.rgb, mrgLocalLight3_Range);
#endif

    return result;
}

float3 ComputeLightField(float3 normal)
{
    float3 squared = normal * normal;
    int3 positive = normal >= 0.0;

    return
        squared.x * g_aLightField[positive.x] +
        squared.y * g_aLightField[positive.y + 2] +
        squared.z * g_aLightField[positive.z + 4];
}

void main(in DECLARATION_TYPE input,
    out float4 outColor0 : COLOR0,
    out float4 outColor1 : COLOR1,
    out float4 outColor2 : COLOR2,
    out float4 outColor3 : COLOR3)
{
#if defined(NoGIOnly) && NoGIOnly && (!defined(NoGI) || !NoGI) || \
    defined(GIOnly) && GIOnly && defined(NoGI) && NoGI
    outColor0 = float4(1, 0, 0, 1);
    return;
#endif

    Material material = NewMaterial();

#if (defined(HasNormal) && HasNormal)
    float3x3 worldToTangentMatrix = float3x3(input.Tangent.xyz, input.Binormal.xyz, input.Normal.xyz);
    float3x3 tangentToWorldMatrix = transpose(worldToTangentMatrix);

    material.Normal = normalize(g_DebugParam[0].y >= 0 ? input.Normal.xyz : GetNormal(input, tangentToWorldMatrix));
#else
    material.Normal = normalize(input.Normal);
#endif
    
    float4 diffuse = GetDiffuse(input, material);
    float4 specular = GetSpecular(input);

    material.Albedo = diffuse.rgb;
    material.Alpha = diffuse.a;

    material.Reflectance = specular.x;
    material.Roughness = max(0.01, 1 - specular.y);
    material.AmbientOcclusion = specular.z;
    material.Metalness = specular.w;

#if !defined(HasMetalness) || !HasMetalness
    material.Metalness = material.Reflectance > 0.225;
#endif

    material.ViewDirection = normalize(g_EyePosition.xyz - input.Position.xyz);
    material.CosViewDirection = saturate(dot(material.ViewDirection, material.Normal));

    material.RoughReflectionDirection = ComputeRoughReflectionDirection(material.Roughness, material.Normal, material.ViewDirection);
    material.SmoothReflectionDirection = 2 * material.CosViewDirection * material.Normal - material.ViewDirection;

    PostProcessMaterial(input, material);

    if (g_DebugParam[0].x >= 0) material.Albedo = 1.0;
    if (g_DebugParam[0].z >= 0) material.Reflectance = g_DebugParam[0].z;
    if (g_DebugParam[0].w >= 0) material.Roughness = g_DebugParam[0].w;
    if (g_DebugParam[1].x >= 0) material.AmbientOcclusion = g_DebugParam[1].x;
    if (g_DebugParam[1].y >= 0) material.Metalness = g_DebugParam[1].y;

    material.F0 = lerp(material.Reflectance, material.Albedo, material.Metalness);

#if defined(NoGI) && NoGI
    float sggiBlendFactor = 0.0;
    float iblBlendFactor = 1.0;

    material.IndirectDiffuse = 0;
    material.IndirectSpecular = 0;
    material.Shadow = 1;
#else
    float sggiBlendFactor = saturate(material.Roughness * g_HDRParam_SGGIParam.w + g_HDRParam_SGGIParam.z);
    float iblBlendFactor = lerp(1 - sggiBlendFactor, 1, material.Metalness);

    const float2 giCoord = input.TexCoord0.zw * mrgGIAtlasParam.xy + mrgGIAtlasParam.zw;
    const float2 occlusionCoord = input.TexCoord0.zw * mrgOcclusionAtlasParam.xy + mrgOcclusionAtlasParam.zw;

    [branch] if (mrgIsSG)
    {
        const float2 sggiCoord[4] =
        {
            giCoord + float2(0, 0),
            giCoord + float2(mrgGIAtlasParam.x, 0),
            giCoord + float2(0, mrgGIAtlasParam.y),
            giCoord + float2(mrgGIAtlasParam.x, mrgGIAtlasParam.y)
        };

        float4 sggi[4];

        int i;

        [unroll] for (i = 0; i < 4; i++)
            sggi[i] = tex2Dlod(g_GISampler, float4(sggiCoord[i], 0, 0));

        [branch] if (mrgHasOcclusion)
        {
            [unroll] for (i = 0; i < 4; i++)
                sggi[i].rgb = UnpackHDR(sggi[i]);

            material.Shadow = tex2Dlod(g_OcclusionSampler, float4(occlusionCoord, 0, 0)).x;
        }
        else 
        {
            material.Shadow = tex2Dlod(g_GISampler, float4(occlusionCoord, 0, 0)).a;
        }

        const float3 axis[4] =
        {
            float3(0, 0.57735002, 1),
            float3(0, 0.57735002, -1),
            float3(1, 0.57735002, 0),
            float3(-1, 0.57735002, 0)
        };

        material.IndirectDiffuse = 0;

        [unroll] for (i = 0; i < 4; i++)
            material.IndirectDiffuse += ComputeSggiDiffuse(material, sggi[i].rgb, axis[i]);

        material.IndirectDiffuse *= ComputeSggiDiffuseFactor();

        float4 r6;
        float sggiSpecularFactor = ComputeSggiSpecularFactor(material, r6);

        material.IndirectSpecular = 0;

        [unroll] for (i = 0; i < 4; i++)
            material.IndirectSpecular += ComputeSggiSpecular(material, sggi[i].rgb, axis[i], r6);

        material.IndirectSpecular *= sggiSpecularFactor;
        material.IndirectSpecular *= sggiBlendFactor;
    }
    else
    {
        float4 gi = tex2Dlod(g_GISampler, float4(giCoord, 0, 0));

        [branch] if (mrgHasOcclusion)
        {
            material.IndirectDiffuse = UnpackHDR(gi);
            material.Shadow = tex2Dlod(g_OcclusionSampler, float4(occlusionCoord, 0, 0)).x;
        }
        else
        {
            material.IndirectDiffuse = gi.rgb * gi.rgb;
            material.Shadow = gi.a;
        }
    }

    if (g_DebugParam[1].z >= 0)
    {
        material.IndirectDiffuse = float3(g_DebugParam[1].zw, g_DebugParam[2].x);
        material.IndirectSpecular = 0;
    }
    if (g_DebugParam[2].y >= 0) material.Shadow = g_DebugParam[2].y;

    material.IndirectDiffuse *= g_GI0Scale.rgb;
    material.IndirectSpecular *= g_GI0Scale.rgb;
#endif

    float cosTheta = dot(-mrgGlobalLight_Direction.xyz, material.Normal);

    uint type = PRIMITIVE_TYPE_RAW;

    if (!mrgIsUseDeferred)
    {
#if defined(NoGI) && NoGI
        material.IndirectDiffuse = ComputeLightField(material.Normal);
        material.Shadow *= g_aLightField[0].w;
#endif

        material.IndirectSpecular += UnpackHDR(texCUBElod(g_DefaultIBLSampler, float4(material.RoughReflectionDirection * float3(1, 1, -1), material.Roughness * 3))) * iblBlendFactor;
        material.Shadow *= ComputeShadow(g_ShadowMapSampler, input.ShadowMapCoord, g_ESMParam.xy, g_ESMParam.z);

        float3 directLighting = ComputeDirectLightingRaw(material, -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb);

#if defined(HasCdr) && HasCdr
        directLighting *= GetCdr(cosTheta, input.Color.y);
#else
        directLighting *= saturate(cosTheta);
#endif
        directLighting *= material.Shadow;

        directLighting += ComputeLocalLight(input, material);

#if defined(UseApproxEnvBRDF) && UseApproxEnvBRDF
        float3 indirectLighting = ComputeIndirectLighting(material);
#else
        float3 indirectLighting = ComputeIndirectLighting(material, g_EnvBRDFSampler);
#endif

        outColor0 = float4(directLighting + indirectLighting, material.Alpha) * g_ForceAlphaColor;
        PostProcessFinalColor(input, material, false, outColor0);

        outColor0.rgb = outColor0.rgb * input.ExtraParams.x + g_LightScatteringColor.rgb * input.ExtraParams.y;
    }
    else
    {
        float3 color = 0;

#if defined(NoGI) && NoGI
#if defined(HasCdr) && HasCdr
        color = GetCdr(cosTheta, input.Color.y);
        type = PRIMITIVE_TYPE_CDR;
#else
        type = PRIMITIVE_TYPE_NO_GI;
#endif
#else
#if defined(UseApproxEnvBRDF) && UseApproxEnvBRDF
        color = ComputeIndirectLighting(material);
#else
        color = ComputeIndirectLighting(material, g_EnvBRDFSampler);
#endif
        type = PRIMITIVE_TYPE_GI;
#endif

        outColor0 = float4(color, material.Alpha);
        PostProcessFinalColor(input, material, true, outColor0);
    }

    outColor1 = float4(material.Albedo, material.Shadow);
    outColor2 = float4(material.Reflectance, material.Roughness, material.AmbientOcclusion, material.Metalness);
    outColor3 = float4(material.Normal * 0.5 + 0.5, PackPrimitiveType(type));
}