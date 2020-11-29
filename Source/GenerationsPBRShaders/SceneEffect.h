#pragma once

struct DebugParam
{
    bool UseWhiteAlbedo;
    bool UseFlatNormal;
    float FresnelFactorOverride;
    float RoughnessOverride;
    float MetalnessOverride;
};

struct GIParam
{
    bool EnableCubicFilter;
    bool EnableInverseToneMap;
    float InverseToneMapFactor;
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
    float Thickness;
    float Saturation;
    float Brightness;
};

class SceneEffect
{
public:
    static DebugParam Debug;
    static GIParam GI;
    static SGGIParam SGGI;
    static ESMParam ESM;
    static RLRParam RLR;

    static void applyPatches();
};
