sampler2D g_FramebufferSampler : register(g_FramebufferRegister);
sampler2D g_LUTSampler : register(g_LUTRegister);

float4 main(in float4 texCoord : TEXCOORD) : COLOR
{
    const float2 g_LUTParam = float2(256, 16);

    float4 color = tex2D(g_FramebufferSampler, texCoord.xy);

    float cell = color.b * (g_LUTParam.y - 1);

    float cellL = floor(cell);
    float cellH = ceil(cell);

    float floatX = 0.5 / g_LUTParam.x;
    float floatY = 0.5 / g_LUTParam.y;
    float rOffset = floatX + color.r / g_LUTParam.y * ((g_LUTParam.y - 1) / g_LUTParam.y);
    float gOffset = floatY + color.g * ((g_LUTParam.y - 1) / g_LUTParam.y);

    float2 lutTexCoordL = float2(cellL / g_LUTParam.y + rOffset, gOffset);
    float2 lutTexCoordH = float2(cellH / g_LUTParam.y + rOffset, gOffset);

    float4 gradedColorL = tex2D(g_LUTSampler, lutTexCoordL);
    float4 gradedColorH = tex2D(g_LUTSampler, lutTexCoordH);

    return lerp(gradedColorL, gradedColorH, frac(cell));
}