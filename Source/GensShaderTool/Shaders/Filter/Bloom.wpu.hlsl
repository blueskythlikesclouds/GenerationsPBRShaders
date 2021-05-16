float3 g_BloomStar_Param1 : register(c151);
sampler2D sampDif0 : register(s0);

float4 main(float4 texCoord : TEXCOORD0) : COLOR
{
    float3 color = clamp(tex2Dlod(sampDif0, float4(texCoord.xy, 0, 0)).rgb, 0, 65504);
    float maxCoeff = max(0.001, max(color.r, max(color.g, color.b)));
    float factor = saturate((maxCoeff - g_BloomStar_Param1.x) / (g_BloomStar_Param1.y - g_BloomStar_Param1.x));
    return float4(color / maxCoeff * factor, 1);
}