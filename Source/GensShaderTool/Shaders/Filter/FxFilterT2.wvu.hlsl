#include "../../global.vsparam.hlsl"

void main(in float2 fPosition : POSITION, in float2 fTexCoord : TEXCOORD, out float4 oPosition : POSITION, out float2 oTexCoord : TEXCOORD)
{
    oPosition = float4(fPosition + g_ViewportSize.zw * float2(-1, 1), 0, 1);
    oTexCoord = fTexCoord + g_ViewportSize.zw * float2(-0.5, -0.5);
}