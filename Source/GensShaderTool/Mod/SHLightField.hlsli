#ifndef SH_LIGHT_FIELD_HLSLI_INCLUDED
#define SH_LIGHT_FIELD_HLSLI_INCLUDED

#ifndef GLOBALS_PS_HLSLI_INCLUDED
#include "GlobalsPS.hlsli"
#endif

#include "Shared.hlsli"

void ComputeSHLightField(inout ShaderParams params, in float3 position)
{
    float currentLength = -1;
    int currentIndex = 0;
    float3 currentShCoords;

    int i;

    [unroll] for (i = 0; i < 3; i++)
    {
        float3 shCoords = mul(mrgSHLightFieldMatrices[i], float4(position * 10.0f, 1)).xyz;

        if (IsInSHCoordRange(shCoords))
        {
            currentIndex = i;
            currentShCoords = shCoords;
            break;
        }

        float length = dot(shCoords, shCoords);

        if (i == 0 || length < currentLength)
        {
            currentLength = length;
            currentIndex = i;
            currentShCoords = shCoords;
        }
    }

    currentShCoords = ComputeSHTexCoords(currentShCoords, mrgSHLightFieldParams[currentIndex]);

    float4 shlf[6];

    switch (currentIndex)
    {
    case 0:
    default:
        for (i = 0; i < 6; i++)
            shlf[i] = g_SHLightFieldTextures[0].Sample(g_LinearClampSampler, float4(currentShCoords + float3(i / 9.0f, 0, 0), 0));

        break;

    case 1:
        for (i = 0; i < 6; i++)
            shlf[i] = g_SHLightFieldTextures[1].Sample(g_LinearClampSampler, float4(currentShCoords + float3(i / 9.0f, 0, 0), 0));

        break;

    case 2:
        for (i = 0; i < 6; i++)
            shlf[i] = g_SHLightFieldTextures[2].Sample(g_LinearClampSampler, float4(currentShCoords + float3(i / 9.0f, 0, 0), 0));

        break;
    }

    const float3 directions[6] =
    {
        float3(1, 0, 0),
        float3(-1, 0, 0),
        float3(0, 1, 0),
        float3(0, -1, 0),
        float3(0, 0, 1),
        float3(0, 0, -1)
    };

    [unroll] for (i = 0; i < 6; i++)
        params.IndirectDiffuse += ComputeSHBasis(params, shlf[i], directions[i]);

    params.IndirectDiffuse = ComputeSHFinal(params.IndirectDiffuse) * g_GI0Scale.rgb;
    params.Shadow *= shlf[0].a;
}

#endif