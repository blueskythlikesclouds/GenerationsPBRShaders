#define GLOBALS_PS_APPEND_PARAMETERS_BEGIN
#include "../GlobalsPS.hlsli"

float4 g_MiddleGray_Scale_LuminanceLow_LuminanceHigh : packoffset(c150);

GLOBALS_PS_APPEND_PARAMETERS_END

Texture2D<float4> texDif0 : register(t0);
Texture2D<float4> texDif1 : register(t1);

SamplerState sampDif0 : register(s0);
SamplerState sampDif1 : register(s1);

float GranTurismoToneMap(float x, float P, float a, float m, float l, float c, float b)
{
    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0 - m) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    float w0 = 1.0 - smoothstep(0.0, m, x);
    float w2 = step(m + l0, x);
    float w1 = 1.0 - w0 - w2;

    float T = m * pow(abs(x / m), c) + b;
    float S = P - (P - S1) * exp(CP * (x - S0));
    float L = m + a * (x - m);

    return T * w0 + L * w1 + S * w2;
}

float GranTurismoToneMap(float x)
{
    const float P = 1.1;  // max display brightness
    const float a = 1.4;  // contrast
    const float m = 0.1;  // linear section start
    const float l = 0.35; // linear section length
    const float c = 1.2;  // black
    const float b = 0.0;  // pedestal
    return GranTurismoToneMap(x, P, a, m, l, c, b);
}

float4 main(in float4 svPosition : SV_POSITION, in float2 texCoord : TEXCOORD) : SV_Target0
{
    float avgLum = texDif1.Load(0).x;
    float rcpLum = g_MiddleGray_Scale_LuminanceLow_LuminanceHigh.x / (0.001 + avgLum);

    float3 color = texDif0.Sample(sampDif0, texCoord).rgb * rcpLum;

    color.r = GranTurismoToneMap(color.r);
    color.g = GranTurismoToneMap(color.g);
    color.b = GranTurismoToneMap(color.b);

    return float4(color, 1.0);
}