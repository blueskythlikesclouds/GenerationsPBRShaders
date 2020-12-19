#include "../global.psparam.hlsl"
#include "Functions.hlsl"

float4 Scale : register(c150);

sampler2D diffuseSampler : register(s0);
sampler2D transparencySampler : register(s1);

float4 main(in float2 vPos : VPOS, in float4 texCoord0 : TEXCOORD0, in float4 texCoord1 : TEXCOORD1) : COLOR
{
    float2 screenPos = (vPos + 0.5) * g_ViewportSize.zw;
    
    if (tex2Dlod(g_DepthSampler, float4(screenPos, 0, 0)).x < 1.0)
        return tex2Dlod(g_FramebufferSampler, float4(screenPos, 0, 0));

    float4 diffuse = tex2D(diffuseSampler, texCoord0.xy);

    diffuse.rgb = UnpackHDRCustom(diffuse.rgb, Scale.x) * g_BackGroundScale.x;

#if defined(HasTransparency) && HasTransparency
    diffuse.a *= tex2D(transparencySampler, texCoord0.xy).x;
#endif

    return diffuse;
}