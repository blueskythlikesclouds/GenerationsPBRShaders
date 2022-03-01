#ifndef UTILITIES_HLSLI_INCLUDED
#define UTILITIES_HLSLI_INCLUDED

float3 SrgbToLinear(float3 value)
{
    return pow(value, 2.2);
}

float4 SrgbToLinear(float4 value)
{
    return float4(SrgbToLinear(value.rgb), value.a);
}

float3 LinearToSrgb(float3 value)
{
    return pow(value, 1.0 / 2.2);
}

float4 LinearToSrgb(float4 value)
{
    return float4(LinearToSrgb(value.rgb), value.a);
}

#endif