sampler2D g_FramebufferSampler : register(g_FramebufferRegister);
sampler2D g_LUTSampler : register(g_LUTRegister);

float4 main(in float4 texCoord : TEXCOORD) : COLOR
{
    const float2 g_LUTParam = float2(256, 16);

    float4 color = tex2Dlod(g_FramebufferSampler, float4(texCoord.xy, 0, 0));

    float cell = color.b * (g_LUTParam.y - 1);

    float cellL = floor(cell);
    float cellH = ceil(cell);

    float floatX = 0.5 / g_LUTParam.x;
    float floatY = 0.5 / g_LUTParam.y;
    float rOffset = floatX + color.r / g_LUTParam.y * ((g_LUTParam.y - 1) / g_LUTParam.y);
    float gOffset = floatY + color.g * ((g_LUTParam.y - 1) / g_LUTParam.y);

    float2 lutTexCoordL = float2(cellL / g_LUTParam.y + rOffset, gOffset);
    float2 lutTexCoordH = float2(cellH / g_LUTParam.y + rOffset, gOffset);

    float4 gradedColorL = tex2Dlod(g_LUTSampler, float4(lutTexCoordL, 0, 0));
    float4 gradedColorH = tex2Dlod(g_LUTSampler, float4(lutTexCoordH, 0, 0));

    return lerp(gradedColorL, gradedColorH, frac(cell));
}