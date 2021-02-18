#include "../global.psparam.hlsl"
#include "Material/Default.hlsl"
#include "Declarations.hlsl"

float4 Luminance : register(c150);
float4 Offset : register(c151);

sampler2D diffuseSampler : register(s0);
sampler2D emissionSampler : register(s1);
sampler2D transparencySampler : register(s2);
sampler2D reflectionSampler : register(s3);

void main(
    in DefaultDeclaration input,
    out float4 outColor0 : COLOR0,
    out float4 outColor1 : COLOR1,
    out float4 outColor2 : COLOR2,
    out float4 outColor3 : COLOR3)
{
    // TODO: Figure what the reflection sampler is for

    float4 diffuse;

#if defined(HasDiffuse) && HasDiffuse
    diffuse = pow(tex2D(diffuseSampler, UV(0)), GAMMA) * float4(g_Diffuse.rgb, 1.0) * input.Color;
#else
    diffuse = float4(0, 0, 0, 1);
#endif

    diffuse.rgb *= g_MiddleGray_Scale_LuminanceLow_LuminanceHigh.x * 
        g_MiddleGray_Scale_LuminanceLow_LuminanceHigh.w;

#if defined(HasTransparency) && HasTransparency
    diffuse.a *= tex2D(transparencySampler, UV(2)).a;
#endif

    float3 emission;

#if defined(HasEmissionReflection) && HasEmissionReflection
    float3 viewNormal = normalize(mul(float4(input.Normal.xyz, 0), g_MtxView).xyz);
    emission = tex2D(emissionSampler, viewNormal.xy * float2(0.5, -0.5) + 0.5).rgb;
#elif defined(HasEmission) && HasEmission
    emission = tex2D(emissionSampler, UV(1)).rgb;
#else
    emission = g_Emissive.rgb;
#endif

    emission *= g_Ambient.rgb * Luminance.x;

    float3 result = diffuse.rgb + emission.rgb;

    if (!mrgIsUseDeferred)
    {
        outColor0.rgb = result * input.ExtraParams.x + g_LightScatteringColor.rgb * input.ExtraParams.y;
        outColor0.a = diffuse.a;
    }
    else
    {
        outColor0 = float4(result, diffuse.a);
        outColor1 = 0;
        outColor2 = float4(0, 1, 0, 1);
        outColor3 = float4(normalize(input.Normal.xyz) * 0.5 + 0.5, PackPrimitiveType(PRIMITIVE_TYPE_EMISSION));
    }
}