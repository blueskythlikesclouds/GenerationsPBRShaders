#include "GlobalsVS.hlsli"
#include "HeightMapShared.hlsli"

Texture2D<float4> g_HeightMapTexture : register(t0);
SamplerState g_HeightMapSampler : register(s0);

void main(in HeightMapVSDeclaration i, out HeightMapPSDeclaration o)
{
    o.Position = mul(float4(i.Position, 1.0), g_MtxWorld).xyz;
    o.TexCoord = o.Position.xz / 4096.0 + 0.5;
    o.Position.y = g_HeightMapTexture.SampleLevel(g_HeightMapSampler, o.TexCoord, 0).x * 1000.0;
    o.SVPosition = mul(mul(float4(o.Position, 1.0), g_MtxView), g_MtxProjection);
}