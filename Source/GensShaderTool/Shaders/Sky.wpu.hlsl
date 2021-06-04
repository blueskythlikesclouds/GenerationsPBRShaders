#include "../global.psparam.hlsl"
#include "Functions.hlsl"

sampler2D diffuseSampler : register(s0);
sampler2D transparencySampler : register(s1);

float4 main(in float2 vPos : VPOS, in float4 texCoord0 : TEXCOORD0, in float4 texCoord1 : TEXCOORD1, in float4 color : COLOR) : COLOR
{
    float4 diffuse = tex2D(diffuseSampler, texCoord0.xy);

#if defined(IsSky3) && IsSky3
    diffuse = pow(diffuse * color, GAMMA);
#else
    diffuse.rgb = UnpackHDR(diffuse);
    diffuse.a = 1;
#endif

    diffuse.rgb *= g_BackGroundScale.x;

#if defined(HasTransparency) && HasTransparency
    diffuse.a *= tex2D(transparencySampler, texCoord0.xy).x;
#endif

    return diffuse;
}