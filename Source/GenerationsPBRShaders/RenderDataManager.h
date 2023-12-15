#pragma once

class NodeBVH;

struct SHLightFieldData
{
    OBB obb;
    Eigen::Matrix4f inverseMatrix;
    Eigen::AlignedVector3f position;
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
    Eigen::AlignedVector3f position;
    float bias;
    float radius;
    float distance;
    size_t pictureIndex;
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
    Eigen::AlignedVector3f position;
    Eigen::AlignedVector3f color;
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

    static size_t iblProbesMipLevels;
    static size_t defaultIblMipLevels;
    static bool defaultIblExposurePacked;
    static ComPtr<ID3D11Texture2D> iblProbesTex;
    static ComPtr<ID3D11ShaderResourceView> iblProbesSRV;
    static boost::shared_ptr<hh::ygg::CYggPicture> iblProbesPicture;

    static std::vector<std::unique_ptr<SHLightFieldData>> shlfs;
    static std::vector<std::unique_ptr<IBLProbeData>> iblProbes;
    static std::vector<std::unique_ptr<LightMotionData>> lightMotions;
    static std::vector<std::unique_ptr<LocalLightData>> localLights;

    static std::vector<const SHLightFieldData*> shlfsInFrustum;
    static std::vector<const IBLProbeData*> iblProbesInFrustum;
    static std::vector<const LocalLightData*> localLightsInFrustum;

    static NodeBVH nodeBVH;

    static boost::shared_ptr<hh::ygg::CYggPicture> heightMapPicture;
    static ComPtr<ID3D11ShaderResourceView> heightMapSRV;

    static void applyPatches();
};
