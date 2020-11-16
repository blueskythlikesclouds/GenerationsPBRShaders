sampler2D g_TV : register(s11);

float4 main(in float2 texCoord : TEXCOORD) : COLOR
{
    return tex2D(g_TV, float2(1 - texCoord.y, texCoord.x));
}