﻿#pragma once

class NodeBVH;

struct SHLightFieldData
{
    OBB obb;
    Eigen::Matrix4f inverseMatrix;
    Eigen::Vector3f position;
    uint32_t probeCounts[3];
    float radius;
    float distance;
    boost::shared_ptr<hh::ygg::CYggPicture> picture;
};

struct IBLProbeData
{
    std::string name;
    OBB obb;
    Eigen::Matrix4f inverseMatrix;
    Eigen::Vector3f position;
    float bias;
    float radius;
    float distance;
    boost::shared_ptr<hh::ygg::CYggPicture> picture;
};

struct LightMotionData
{
    std::string name;
    boost::shared_ptr<hh::mot::CLightMotionData> data;
    hh::mot::CLightSubMotionValueData valueData;
};

struct LocalLightData
{
    boost::shared_ptr<hh::mr::CLightData> lightData;
    Eigen::Vector3f position;
    Eigen::Vector3f color;
    Eigen::Vector4f range;
    float distance;
    LightMotionData* lightMotionData;
};

class RenderDataManager
{
    static bool enabled;

public:
    static boost::shared_ptr<hh::ygg::CYggPicture> defaultIBLPicture;
    static boost::shared_ptr<hh::ygg::CYggPicture> rgbTablePicture;

    static std::vector<std::unique_ptr<SHLightFieldData>> shlfs;
    static std::vector<std::unique_ptr<IBLProbeData>> iblProbes;
    static std::vector<std::unique_ptr<LightMotionData>> lightMotions;
    static std::vector<std::unique_ptr<LocalLightData>> localLights;

    static std::vector<const SHLightFieldData*> shlfsInFrustum;
    static std::vector<const IBLProbeData*> iblProbesInFrustum;
    static std::vector<const LocalLightData*> localLightsInFrustum;

    static NodeBVH nodeBVH;

    static void applyPatches();
};
