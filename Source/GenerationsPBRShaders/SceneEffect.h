#pragma once

enum DebugViewMode : uint32_t
{
    DEBUG_VIEW_MODE_NONE,
    DEBUG_VIEW_MODE_GI_ONLY,
    DEBUG_VIEW_MODE_IBL_ONLY,
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
    bool useWhiteAlbedo;
    bool useFlatNormal;
    float reflectanceOverride;
    float smoothnessOverride;
    float ambientOcclusionOverride;
    float metalnessOverride;
    Eigen::Vector3f giColorOverride;
    float giShadowMapOverride;
    DebugViewMode viewMode;
    bool disableDirectLight;
    bool disableLocalLight;
    bool disableSHLightField;
    bool disableDefaultIBL;
    bool disableIBLProbe;
    bool disableLUT;
    unsigned long maxProbeCount;
};

struct CullingParam
{
    float shLightFieldCullingRange;
    float iblProbeCullingRange;
    float localLightCullingRange;
};

struct SGGIParam
{
    float startSmoothness;
    float endSmoothness;
};

struct ESMParam
{
    float factor;
};

struct RLRParam
{
    bool enable;
    unsigned long stepCount;
    float maxRoughness;
    float rayLength;
    float fade;
    float accuracyThreshold;
    float saturation;
    float brightness;
    int32_t maxLod;
};

struct HighlightParam
{
    bool enable;
    float threshold;
    float objectAmbientScale;
    float objectAlbedoHeighten;
    float charaAmbientScale;
    float charaAlbedoHeighten;
    float charaFalloffScale;
};

struct SSAOParam
{
    bool enable;
    unsigned long sampleCount;
    float radius;
    float distanceFade;
    float strength;
    float depthThreshold;
};

enum BloomType : uint32_t
{
    BLOOM_TYPE_DEFAULT,
    BLOOM_TYPE_PBR,
    BLOOM_TYPE_BFXP,
    BLOOM_TYPE_COLORS
};

struct BloomParam
{
    BloomType type;
};

class SceneEffect
{
public:
    static DebugParam debug;
    static CullingParam culling;
    static SGGIParam sggi;
    static ESMParam esm;
    static RLRParam rlr;
    static HighlightParam highlight;
    static SSAOParam ssao;
    static BloomParam bloom;

    static void applyPatches();
};
