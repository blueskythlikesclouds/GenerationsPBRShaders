#ifndef GLOBALS_SHARED_HLSLI_INCLUDED
#define GLOBALS_SHARED_HLSLI_INCLUDED

cbuffer cb_GlobalsShared : register(b2)
{
    float g_ReflectanceOverride;
    float g_SmoothnessOverride;
    float g_AmbientOcclusionOverride;
    float g_MetalnessOverride;

    float3 g_GIColorOverride;
    float g_GIShadowOverride;

    float2 g_SGGIParam;
    float g_HDRParam;
    float g_ESMParam;

    bool g_UseWhiteAlbedo;
    bool g_UseFlatNormal;

    bool g_UsePBR;
    bool g_IsUseDebugParam;
}

#endif
