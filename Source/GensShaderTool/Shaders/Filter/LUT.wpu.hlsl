sampler2D s0 : register(s0);
sampler2D s1 : register(s1);

bool g_IsEnableLUT : register(b8);

float4 main(in float4 texCoord : TEXCOORD) : COLOR
{
    const float2 g_LUTParam = float2(256, 16);

    float4 color = tex2Dlod(s0, float4(texCoord.xy, 0, 0));
    color.rgb = pow(abs(color.rgb), 1.0 / 2.2);

    if (g_IsEnableLUT)
    {
        float cell = color.b * (g_LUTParam.y - 1);

        float cellL = floor(cell);
        float cellH = ceil(cell);

        float floatX = 0.5 / g_LUTParam.x;
        float floatY = 0.5 / g_LUTParam.y;
        float rOffset = floatX + color.r / g_LUTParam.y * ((g_LUTParam.y - 1) / g_LUTParam.y);
        float gOffset = floatY + color.g * ((g_LUTParam.y - 1) / g_LUTParam.y);

        float2 lutTexCoordL = float2(cellL / g_LUTParam.y + rOffset, gOffset);
        float2 lutTexCoordH = float2(cellH / g_LUTParam.y + rOffset, gOffset);

        float4 gradedColorL = tex2Dlod(s1, float4(lutTexCoordL, 0, 0));
        float4 gradedColorH = tex2Dlod(s1, float4(lutTexCoordH, 0, 0));

        color.rgb = lerp(gradedColorL, gradedColorH, frac(cell)).rgb;
    }

    return color;
}