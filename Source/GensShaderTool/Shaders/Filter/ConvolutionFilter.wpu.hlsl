// https://github.com/Jam3/glsl-fast-gaussian-blur/blob/master/5.glsl

float4 g_Param : register(c150);
float4 g_Direction : register(c151);

sampler2D s0 : register(s0);

float4 main(in float2 texCoord : TEXCOORD) : COLOR
{
    float4 color = 0.0;
    float2 off1 = 1.3333333333333333 * g_Direction.xy;
    color += max(0, tex2Dlod(s0, float4(texCoord * g_Param.z,                     0, g_Param.w)) * 0.29411764705882354);
    color += max(0, tex2Dlod(s0, float4(texCoord * g_Param.z + off1 * g_Param.xy, 0, g_Param.w)) * 0.35294117647058826);
    color += max(0, tex2Dlod(s0, float4(texCoord * g_Param.z - off1 * g_Param.xy, 0, g_Param.w)) * 0.35294117647058826);
    return color;
}