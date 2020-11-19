#include "../global.psparam.hlsl"

#include "Filter.hlsl"
#include "Functions.hlsl"
#include "Material.hlsl"

#if (defined(IsCommon2) && IsCommon2) || (defined(IsMCommon) && IsMCommon)
#include "Material/Common.hlsl"
#elif (defined(IsBlend2) && IsBlend2) || (defined(IsMBlend) && IsMBlend)
#include "Material/Blend.hlsl"
#elif defined(IsChrEyeCDRF) && IsChrEyeCDRF
#include "Material/Eye.hlsl"
#elif defined(IsChrSkinCDRF) && IsChrSkinCDRF
#include "Material/Skin.hlsl"
#elif defined(IsWater01) && IsWater01
#include "Material/Water01.hlsl"
#elif defined(IsWater05) && IsWater05
#include "Material/Water05.hlsl"
#endif

float4 g_GIParam : register(c189);
float4 g_SGGIParam : register(c190);

float4 g_GIAtlasSize : register(c191);
float4 g_ShadowMapSize : register(c192);

bool g_IsUseDeferred : register(b6);
bool g_IsUseSGGI : register(b7);
bool g_IsUseCubicFilter : register(b8);

samplerCUBE g_DefaultIBLSampler : register(s14);
sampler2D g_EnvBRDFSampler : register(s15);

float4 texGI(float2 texCoord, float2 gradX, float2 gradY)
{
    [branch] if (g_IsUseCubicFilter)
        return tex2DgradFastBicubic(g_GISampler, texCoord.x * g_GIAtlasSize.x, texCoord.y * g_GIAtlasSize.y, g_GIAtlasSize.zw, gradX, gradY);

    return tex2Dgrad(g_GISampler, texCoord, gradX, gradY);
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

    Material material;

#if (defined(HasNormal) && HasNormal)
    float3x3 worldToTangentMatrix = float3x3(input.Tangent.xyz, input.Binormal.xyz, input.Normal.xyz);
    float3x3 tangentToWorldMatrix = transpose(worldToTangentMatrix);

    material.Normal = normalize(GetNormal(input, tangentToWorldMatrix));
#else
    material.Normal = normalize(input.Normal);
#endif
    
    float4 diffuse = GetDiffuse(input, material);
    float4 specular = GetSpecular(input);

    material.Albedo = diffuse.rgb;
    material.Alpha = diffuse.a;

    material.FresnelFactor = specular.x;
    material.Roughness = max(0.01, 1 - specular.y);
    material.AmbientOcclusion = specular.z;
    material.Metalness = specular.w;

#if !defined(HasMetalness) || !HasMetalness
    material.Metalness = material.FresnelFactor > 0.225;
#endif

    material.ViewDirection = normalize(g_EyePosition.xyz - input.Position.xyz);
    material.CosViewDirection = saturate(dot(material.ViewDirection, material.Normal));

    material.ReflectionDirection = 2 * material.CosViewDirection * material.Normal - material.ViewDirection;
    material.CosReflectionDirection = saturate(dot(material.ReflectionDirection, material.Normal));

    material.F0 = lerp(material.FresnelFactor, material.Albedo, material.Metalness);

    float sggiRoughness = saturate((material.Roughness - 0.3) * 2.85714);

#if defined(NoGI) && NoGI
    material.IndirectDiffuse = 0;
    material.IndirectSpecular = 0;
    material.Shadow = 1;
#else
    float2 giCoord = input.TexCoord0.zw * g_GIParam.xy + g_GIParam.zw;

    float2 gradX = ddx(giCoord);
    float2 gradY = ddy(giCoord);

    float4 gi = texGI(giCoord, gradX, gradY);

    [branch] if (g_IsUseSGGI)
    {
        float2 sggiCoord = input.TexCoord0.zw * g_SGGIParam.xy + g_SGGIParam.zw;

        float2 sggiCoord0 = sggiCoord + float2(0,             0);
        float2 sggiCoord1 = sggiCoord + float2(g_SGGIParam.x, 0);
        float2 sggiCoord2 = sggiCoord + float2(0,             g_SGGIParam.y);
        float2 sggiCoord3 = sggiCoord + float2(g_SGGIParam.x, g_SGGIParam.y);

        float3 sggiLevel0 = UnpackHDRCustom(texGI(sggiCoord0, gradX, gradY).rgb, g_GI0Scale.x);
        float3 sggiLevel1 = UnpackHDRCustom(texGI(sggiCoord1, gradX, gradY).rgb, g_GI0Scale.x);
        float3 sggiLevel2 = UnpackHDRCustom(texGI(sggiCoord2, gradX, gradY).rgb, g_GI0Scale.x);
        float3 sggiLevel3 = UnpackHDRCustom(texGI(sggiCoord3, gradX, gradY).rgb, g_GI0Scale.x);

        const float3 sggiAxis0 = float3(0, 0.57735002, 1);
        const float3 sggiAxis1 = float3(0, 0.57735002, -1);
        const float3 sggiAxis2 = float3(1, 0.57735002, 0);
        const float3 sggiAxis3 = float3(-1, 0.57735002, 0);

        material.IndirectDiffuse =  ComputeSggiDiffuse(material, sggiLevel0, sggiAxis0);
        material.IndirectDiffuse += ComputeSggiDiffuse(material, sggiLevel1, sggiAxis1);
        material.IndirectDiffuse += ComputeSggiDiffuse(material, sggiLevel2, sggiAxis2);
        material.IndirectDiffuse += ComputeSggiDiffuse(material, sggiLevel3, sggiAxis3);

        material.IndirectDiffuse *= ComputeSggiDiffuseFactor();

        float4 r6;
        float sggiSpecularFactor = ComputeSggiSpecularFactor(material, r6);

        material.IndirectSpecular =  ComputeSggiSpecular(material, sggiLevel0, sggiAxis0, r6);
        material.IndirectSpecular += ComputeSggiSpecular(material, sggiLevel1, sggiAxis1, r6);
        material.IndirectSpecular += ComputeSggiSpecular(material, sggiLevel2, sggiAxis2, r6);
        material.IndirectSpecular += ComputeSggiSpecular(material, sggiLevel3, sggiAxis3, r6);

        material.IndirectSpecular *= sggiSpecularFactor;
        material.IndirectSpecular *= sggiRoughness;
    }
    else
    {
        material.IndirectDiffuse = UnpackHDRCustom(gi.rgb, g_GI0Scale.x);
    }

    material.Shadow = gi.a;
#endif

    PostProcessMaterial(input, material);

    float cosTheta = dot(-mrgGlobalLight_Direction.xyz, material.Normal);

    if (!g_IsUseDeferred)
    {
        material.IndirectSpecular += UnpackHDR(texCUBElod(g_DefaultIBLSampler, float4(material.ReflectionDirection * float3(1, 1, -1), material.Roughness * 3))) * (1 - sggiRoughness);
        material.Shadow *= ComputeShadow(g_ShadowMapSampler, input.ShadowMapCoord, g_ShadowMapSize.xy);

        float3 directLighting = ComputeDirectLightingRaw(material, -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb) * material.Shadow;

#if defined(HasCdr) && HasCdr
        directLighting *= GetCdr(cosTheta, input.Color.y);
#else
        directLighting *= saturate(cosTheta);
#endif
        directLighting *= material.Shadow;

        float3 indirectLighting = ComputeIndirectLighting(material, g_EnvBRDFSampler);

        outColor0 = float4(directLighting + indirectLighting, material.Alpha);

        PostProcessFinalColor(input, material, outColor0);
    }
    else
    {
        float3 color = 0;
        float factor = 0.0;

#if defined(NoGI) && NoGI
#if defined(HasCdr) && HasCdr
        color = GetCdr(cosTheta, input.Color.y);
        factor = 1.0;
#endif

#else
        color = ComputeIndirectLighting(material, g_EnvBRDFSampler);
        factor = material.Shadow;
#endif

        outColor0 = float4(color, material.Alpha);
        outColor1 = float4(material.Albedo, factor);
        outColor2 = float4(material.FresnelFactor, material.Roughness, material.AmbientOcclusion, material.Metalness);
        outColor3 = float4(material.Normal * 0.5 + 0.5, 1.0);
    }
}