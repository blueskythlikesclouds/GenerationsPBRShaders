sampler2D s0 : register( s0 );

float4 main( float4 texCoord : TEXCOORD0 ) : COLOR
{
    float4 color = tex2D(s0, texCoord.xy);
    return saturate(float4(pow(color.rgb, 1.0 / 2.2), color.a));
}