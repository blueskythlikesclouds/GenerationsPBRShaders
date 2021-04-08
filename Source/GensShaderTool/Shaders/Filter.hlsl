#ifndef SCENE_FILTER_INCLUDED
#define SCENE_FILTER_INCLUDED

float texESM(sampler tex, float2 texSize, float2 texCoord, float depth, float esmFactor)
{
    float2 pixelPos = texCoord * texSize.x - 0.5;
    float2 pixelCenter = floor(pixelPos);
    float2 fraction = pixelPos - pixelCenter;
    texCoord = (pixelCenter + 0.5) * texSize.y;

    float4 values;

    values.x = tex2Dlod(tex, float4(texCoord,                                0, 0));
    values.y = tex2Dlod(tex, float4(texCoord + float2(texSize.y, 0),         0, 0));
    values.z = tex2Dlod(tex, float4(texCoord + float2(0,         texSize.y), 0, 0));
    values.w = tex2Dlod(tex, float4(texCoord + float2(texSize.y, texSize.y), 0, 0));
    values = saturate(exp2((values - depth) * esmFactor));

    return lerp(lerp(values.x, values.y, fraction.x), lerp(values.z, values.w, fraction.x), fraction.y);
}

#endif