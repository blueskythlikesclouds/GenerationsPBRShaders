#pragma once

enum DebugViewMode : uint32_t
{
    DEBUG_VIEW_MODE_NONE,
    DEBUG_VIEW_MODE_GI_ONLY,
    DEBUG_VIEW_MODE_GBUFFER1,
    DEBUG_VIEW_MODE_GBUFFER2,
    DEBUG_VIEW_MODE_GBUFFER3,
    DEBUG_VIEW_MODE_RLR,
    DEBUG_VIEW_MODE_SSAO,
    DEBUG_VIEW_MODE_SHADOW_MAP,
    DEBUG_VIEW_MODE_SHADOW_MAP_NO_TERRAIN
};

struct DebugParam
{
    bool UseWhiteAlbedo;
    bool UseFlatNormal;
    float ReflectanceOverride;
    float RoughnessOverride;
    float MetalnessOverride;
    float GIShadowMapOverride;
    DebugViewMode ViewMode;
    bool DisableDirectLight;
    bool DisableOmniLight;
    bool DisableSHLightField;
    bool DisableDefaultIBL;
    bool DisableIBLProbe;
    bool DisableLUT;
    unsigned long MaxProbeCount;
};

struct SGGIParam
{
    float StartSmoothness;
    float EndSmoothness;
};

struct ESMParam
{
    float Factor;
};

struct RLRParam
{
    bool Enable;
    unsigned long StepCount;
    float MaxRoughness;
    float RayLength;
    float Fade;
    float AccuracyThreshold;
    float Saturation;
    float Brightness;
    int32_t MaxLod;
};

struct HighlightParam
{
    bool Enable;
    float Threshold;
    float ObjectAmbientScale;
    float ObjectAlbedoHeighten;
    float CharaAmbientScale;
    float CharaAlbedoHeighten;
    float CharaFalloffScale;
};

struct SSAOParam
{
    bool Enable;
    unsigned long SampleCount;
    float Radius;
    float DistanceFade;
    float Strength;
};

class SceneEffect
{
public:
    static DebugParam Debug;
    static SGGIParam SGGI;
    static ESMParam ESM;
    static RLRParam RLR;
    static HighlightParam Highlight;
    static SSAOParam SSAO;

    static void applyPatches();
};
