// Source: https://github.com/zchee/cuda-sample/blob/master/3_Imaging/bicubicTexture/bicubicTexture_kernel.cuh

float4 g_TextureSize : register(c150);

sampler2D s0 : register(s0);

float w0(float a)
{
    return (1.0f / 6.0f) * (a * (a * (-a + 3.0f) - 3.0f) + 1.0f);
}

float w1(float a)
{
    return (1.0f / 6.0f) * (a * a * (3.0f * a - 6.0f) + 4.0f);
}

float w2(float a)
{
    return (1.0f / 6.0f) * (a * (a * (-3.0f * a + 3.0f) + 3.0f) + 1.0f);
}

float w3(float a)
{
    return (1.0f / 6.0f) * (a * a * a);
}

float g0(float a)
{
    return w0(a) + w1(a);
}

float g1(float a)
{
    return w2(a) + w3(a);
}

float h0(float a)
{
    return -1.0f + w1(a) / (w0(a) + w1(a)) + 0.5f;
}

float h1(float a)
{
    return 1.0f + w3(a) / (w2(a) + w3(a)) + 0.5f;
}

float4 tex2DFastBicubic(sampler2D tex, float x, float y, float2 invSize)
{
    x -= 0.5f;
    y -= 0.5f;
    float px = floor(x);
    float py = floor(y);
    float fx = x - px;
    float fy = y - py;

    float g0x = g0(fx);
    float g1x = g1(fx);
    float h0x = h0(fx);
    float h1x = h1(fx);
    float h0y = h0(fy);
    float h1y = h1(fy);

    float4 r = 
        g0(fy) * (g0x * tex2D(tex, float2(px + h0x, py + h0y) * invSize) +
                  g1x * tex2D(tex, float2(px + h1x, py + h0y) * invSize)) +
        g1(fy) * (g0x * tex2D(tex, float2(px + h0x, py + h1y) * invSize) +
                  g1x * tex2D(tex, float2(px + h1x, py + h1y) * invSize));

    return r;
}

float4 main(in float2 texCoord : TEXCOORD) : COLOR
{
    return tex2DFastBicubic(s0, texCoord.x * g_TextureSize.x, texCoord.y * g_TextureSize.y, g_TextureSize.zw);
}