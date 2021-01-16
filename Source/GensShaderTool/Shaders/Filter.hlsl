// Source: https://github.com/zchee/cuda-sample/blob/master/3_Imaging/bicubicTexture/bicubicTexture_kernel.cuh

#ifndef SCENE_BICUBIC_INCLUDED
#define SCENE_BICUBIC_INCLUDED

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

float4 tex2DFastBicubic(const sampler texref, float x, float y, float2 invSize)
{
    x -= 0.5f;
    y -= 0.5f;
    float px = floor(x);
    float py = floor(y);
    float fx = x - px;
    float fy = y - py;

    // note: we could store these functions in a lookup table texture, but maths is cheap
    float g0x = g0(fx);
    float g1x = g1(fx);
    float h0x = h0(fx);
    float h1x = h1(fx);
    float h0y = h0(fy);
    float h1y = h1(fy);

    float4 r = g0(fy) * (g0x * tex2D(texref, float2(px + h0x, py + h0y) * invSize) +
                         g1x * tex2D(texref, float2(px + h1x, py + h0y) * invSize)) +
               g1(fy) * (g0x * tex2D(texref, float2(px + h0x, py + h1y) * invSize) +
                         g1x * tex2D(texref, float2(px + h1x, py + h1y) * invSize));
    return r;
}

float4 tex2DlodFastBicubic(const sampler texref, float x, float y, float2 invSize, float lod)
{
    x -= 0.5f;
    y -= 0.5f;
    float px = floor(x);
    float py = floor(y);
    float fx = x - px;
    float fy = y - py;

    // note: we could store these functions in a lookup table texture, but maths is cheap
    float g0x = g0(fx);
    float g1x = g1(fx);
    float h0x = h0(fx);
    float h1x = h1(fx);
    float h0y = h0(fy);
    float h1y = h1(fy);

    float4 r = g0(fy) * (g0x * tex2Dlod(texref, float4(float2(px + h0x, py + h0y) * invSize, 0, lod)) +
                         g1x * tex2Dlod(texref, float4(float2(px + h1x, py + h0y) * invSize, 0, lod))) +
               g1(fy) * (g0x * tex2Dlod(texref, float4(float2(px + h0x, py + h1y) * invSize, 0, lod)) +
                         g1x * tex2Dlod(texref, float4(float2(px + h1x, py + h1y) * invSize, 0, lod)));
    return r;
}

float4 tex2DgradFastBicubic(const sampler texref, float x, float y, float2 invSize, float2 gradX, float2 gradY)
{
    x -= 0.5f;
    y -= 0.5f;
    float px = floor(x);
    float py = floor(y);
    float fx = x - px;
    float fy = y - py;

    // note: we could store these functions in a lookup table texture, but maths is cheap
    float g0x = g0(fx);
    float g1x = g1(fx);
    float h0x = h0(fx);
    float h1x = h1(fx);
    float h0y = h0(fy);
    float h1y = h1(fy);

    float4 r = g0(fy) * (g0x * tex2Dgrad(texref, float2(px + h0x, py + h0y) * invSize, gradX, gradY) +
                         g1x * tex2Dgrad(texref, float2(px + h1x, py + h0y) * invSize, gradX, gradY)) +
               g1(fy) * (g0x * tex2Dgrad(texref, float2(px + h0x, py + h1y) * invSize, gradX, gradY) +
                         g1x * tex2Dgrad(texref, float2(px + h1x, py + h1y) * invSize, gradX, gradY));
    return r;
}

float4 tex2DBilinear(sampler tex, float2 uv, float2 size, float2 invSize)
{
	uv *= size;
	uv.x -= 0.5f;
	uv.y -= 0.5f;
	float px = floor(uv.x);
	float py = floor(uv.y);
	float fx = uv.x - px;
	float fy = uv.y - py;
	px += 0.5f;
	py += 0.5f;

	return lerp(lerp(tex2D(tex, float2(px, py) * invSize), tex2D(tex, float2(px + 1.0f, py) * invSize), fx),
		lerp(tex2D(tex, float2(px, py + 1.0f) * invSize), tex2D(tex, float2(px + 1.0f, py + 1.0f) * invSize), fx), fy);
}

float tex2DBilinearESM(sampler tex, float2 uv, float2 size, float2 invSize, float depth, float esmFactor)
{
	uv *= size;
	uv.x -= 0.5f;
	uv.y -= 0.5f;
	float px = floor(uv.x);
	float py = floor(uv.y);
	float fx = uv.x - px;
	float fy = uv.y - py;
	px += 0.5f;
	py += 0.5f;

	float a = tex2Dlod(tex, float4(px, py, 0, 0) * invSize.xyxx).x;
	float b = tex2Dlod(tex, float4(px + 1.0f, py, 0, 0) * invSize.xyxx).x;
	float c = tex2Dlod(tex, float4(px, py + 1.0f, 0, 0) * invSize.xyxx).x;
	float d = tex2Dlod(tex, float4(px + 1.0f, py + 1.0f, 0, 0) * invSize.xyxx).x;

	a = saturate(exp2((a - depth) * esmFactor));
	b = saturate(exp2((b - depth) * esmFactor));
	c = saturate(exp2((c - depth) * esmFactor));
	d = saturate(exp2((d - depth) * esmFactor));

	return lerp(lerp(a, b, fx), lerp(c, d, fx), fy);
}

#endif