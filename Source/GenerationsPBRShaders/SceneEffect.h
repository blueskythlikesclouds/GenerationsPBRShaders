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
    DEBUG_VIEW_MODE_VOLUMETRIC_LIGHTING,
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
    bool renderTerrain;
};

struct RLRParam
{
    bool enable;
    unsigned long stepCount;
    float maxRoughness;
    float rayLength;
    float fade;
    int32_t maxLod;
};

struct SSAOParam : GFSDK_SSAO_Parameters
{
    bool enable{};
};

enum BloomType : uint32_t
{
    BLOOM_TYPE_DEFAULT,
    BLOOM_TYPE_FORCES,
    BLOOM_TYPE_BFXP,
    BLOOM_TYPE_COLORS
};

struct BloomParam
{
    BloomType type;
};

struct VolumetricLightingParam
{
    bool enable;
    bool ignoreSky;
    unsigned long sampleCount;
    float g;
    float inScatteringScale;
    float depthThreshold;
};

struct IBLParam
{
    float defaultIBLIntensity;
    unsigned long maxIBLProbeCount;
};

class SceneEffect
{
public:
    static DebugParam debug;
    static CullingParam culling;
    static SGGIParam sggi;
    static ESMParam esm;
    static RLRParam rlr;
    static SSAOParam ssao;
    static BloomParam bloom;
    static VolumetricLightingParam volumetricLighting;
    static IBLParam ibl;

    static void applyPatches();
};
