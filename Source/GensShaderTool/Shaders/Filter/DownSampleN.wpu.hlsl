float4 g_Param : register(c150);

sampler2D s0 : register(s0);

float4 main(in float2 texCoord : TEXCOORD) : COLOR
{
    float4 sum = 0;
    int count = 0;

    for (float x = -g_Param.x; x <= g_Param.x; x += g_Param.y)
    {
        for (float y = -g_Param.z; y <= g_Param.z; y += g_Param.w)
        {
            sum += tex2Dlod(s0, float4(texCoord + float2(x, y), 0, 0));
            ++count;
        }
    }

    return count > 0 ? sum / count : 0;
}