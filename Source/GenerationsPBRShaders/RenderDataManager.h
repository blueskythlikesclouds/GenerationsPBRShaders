#pragma once

class NodeBVH;

struct SHLightFieldData
{
    OBB m_OBB;
    Eigen::Matrix4f m_InverseMatrix;
    Eigen::Vector3f m_Position;
    uint32_t m_ProbeCounts[3];
    float m_Radius;
    float m_Distance;
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> m_spPicture;
};

struct IBLProbeData
{
    std::string m_Name;
    OBB m_OBB;
    Eigen::Matrix4f m_InverseMatrix;
    Eigen::Vector3f m_Position;
    float m_Bias;
    float m_Radius;
    float m_Distance;
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> m_spPicture;
};

struct LightMotionData
{
    std::string m_Name;
    boost::shared_ptr<Hedgehog::Motion::CLightMotionData> m_spData;
    Hedgehog::Motion::CLightSubMotionValueData m_ValueData;
};

struct LocalLightData
{
    boost::shared_ptr<Hedgehog::Mirage::CLightData> m_spLightData;
    Eigen::Vector3f m_Position;
    Eigen::Vector3f m_Color;
    Eigen::Vector4f m_Range;
    float m_Distance;
    LightMotionData* m_pLightMotionData;
};

class RenderDataManager
{
    static bool enabled;

public:
    static boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> ms_spDefaultIBLPicture;
    static boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> ms_spRgbTablePicture;

    static std::vector<std::unique_ptr<SHLightFieldData>> ms_SHLFs;
    static std::vector<std::unique_ptr<IBLProbeData>> ms_IBLProbes;
    static std::vector<std::unique_ptr<LightMotionData>> ms_LightMotions;
    static std::vector<std::unique_ptr<LocalLightData>> ms_LocalLights;

    static std::vector<const SHLightFieldData*> ms_SHLFsInFrustum;
    static std::vector<const IBLProbeData*> ms_IBLProbesInFrustum;
    static std::vector<const LocalLightData*> ms_LocalLightsInFrustum;

    static NodeBVH ms_NodeBVH;

    static void applyPatches();
};
