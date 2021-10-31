#include "../global.psparam.hlsl"
#include "Functions.hlsl"

sampler2D diffuseSampler : register(s0);
sampler2D transparencySampler : register(s1);
sampler2D emissionSampler : register(s2);

float4 Luminance : register(c150);

float4 main(in float2 vPos : VPOS, in float4 texCoord0 : TEXCOORD0, in float4 texCoord1 : TEXCOORD1, in float4 color : COLOR) : COLOR
{
    float4 diffuse = tex2D(diffuseSampler, texCoord0.xy);

#if defined(IsSky3) && IsSky3
    diffuse *= color;

    diffuse.rgb *= g_Diffuse.rgb;
    diffuse.a *= g_OpacityReflectionRefractionSpectype.x;

#if defined(HasEmission) && HasEmission
    diffuse.rgb += tex2D(emissionSampler, texCoord0.xy).rgb * g_Ambient.rgb * Luminance.x;
#endif

    diffuse = pow(abs(saturate(diffuse)), GAMMA);
#else

#if defined(IsSky2Sqrt) && IsSky2Sqrt
    diffuse.rgb = UnpackHDRSqrt(diffuse);
#else
    diffuse.rgb = UnpackHDR(diffuse);
#endif

    diffuse.a = 1;
#endif

    diffuse.rgb *= g_BackGroundScale.x;

#if defined(HasTransparency) && HasTransparency
    diffuse.a *= tex2D(transparencySampler, texCoord0.xy).x;
#endif

    return diffuse;
}