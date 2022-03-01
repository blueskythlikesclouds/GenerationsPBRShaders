#ifndef DEFAULT_PS_HLSLI_INCLUDED
#define DEFAULT_PS_HLSLI_INCLUDED

#include "Shared.hlsli"

#include "GlobalsShared.hlsli"
#ifndef GLOBALS_PS_HLSLI_INCLUDED
#include "GlobalsPS.hlsli"
#endif

void main(in PixelDeclaration input,

#if defined(IsPermutationDeferred)
    out float4 outColor : SV_TARGET0,
    out float4 gBuffer0 : SV_TARGET1,
    out float4 gBuffer1 : SV_TARGET2,
    out float4 gBuffer2 : SV_TARGET3
#else
    out float4 outColor : SV_TARGET0
#endif

    )
{
    ShaderParams params = CreateShaderParams();
    LoadParams(input, params);

    params.Normal = normalize(input.Normal);

#if defined(HasFeatureNormalMapping)

    if (!g_IsUseDebugParam || !g_UseFlatNormal)
    {
        params.Normal =
            normalize(input.Tangent) * params.NormalMap.x +
            normalize(input.Binormal) * params.NormalMap.y +
            normalize(input.Normal) * sqrt(1 - saturate(dot(params.NormalMap, params.NormalMap)));
    }

#endif

    ModifyParams(input, params);

    // TODO

    outColor.rgb = params.Normal * 0.5 + 0.5;
    outColor.a = 1.0;
}

#endif