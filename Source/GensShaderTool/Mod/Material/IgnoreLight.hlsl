#include "../Shared.hlsli"

#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 Luminance : packoffset(c150);
float4 Offset : packoffset(c151);

GLOBALS_PS_APPEND_PARAMETERS_END

#include "../SharedPS.hlsli"

Texture2D<float4> texDiffuse : register(t0);
Texture2D<float4> texEmission : register(t1);
Texture2D<float> texTransparency : register(t2);
Texture2D<float2> texReflection : register(t3);

SamplerState sampDiffuse : register(s0);
SamplerState sampEmission : register(s1);
SamplerState sampTransparency : register(s2);
SamplerState sampReflection : register(s3);

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
    float2 offset = 0;

#ifdef HasSamplerReflection
    offset = Offset.xy * (texReflection.SampleLevel(sampReflection, UV(3), 0) * 2.0 - 1.0);
#endif

    float4 diffuse;

#ifdef HasSamplerDiffuse
    diffuse = SrgbToLinear(texDiffuse.Sample(sampDiffuse, offset + UV(0))) * float4(g_Diffuse.rgb, 1.0) * input.Color;
#else
    diffuse = float4(0, 0, 0, 1);
#endif

    diffuse.rgb *= GetToneMapLuminance();

#ifdef HasSamplerTransparency
    diffuse.a *= texTransparency.Sample(sampTransparency, offset + UV(2));
#endif

    if (g_EnableAlphaTest)
        clip(diffuse.a - g_AlphaThreshold);

    float3 emission;

#if defined HasSamplerEmissionReflection
    float3 viewNormal = normalize(mul(float4(input.Normal.xyz, 0), g_MtxView).xyz);
    emission = texEmission.Sample(sampEmission, viewNormal.xy * float2(0.5, -0.5) + 0.5).rgb;
#elif defined HasEmission
    emission = texEmission.Sample(sampEmission, offset + UV(1)).rgb;
#else
    emission = g_Emissive.rgb;
#endif

    emission *= g_Ambient.rgb * Luminance.x;

    float3 result = diffuse.rgb + emission.rgb;

#ifndef IsPermutationDeferred
    outColor.rgb = result * input.LightScattering.x + g_LightScatteringColor.rgb * input.LightScattering.y;
    outColor.a = diffuse.a;
#else
    StoreParams(result, normalize(input.Normal), DEFERRED_FLAGS_LIGHT_SCATTERING, gBuffer0, gBuffer1, gBuffer2, gBuffer3);
#endif
}