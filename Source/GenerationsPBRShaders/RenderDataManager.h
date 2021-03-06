#pragma once

template<typename T>
struct DistanceComparePointer
{
    bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs->m_Distance < rhs->m_Distance;
    }
};

template<typename T>
struct DistanceCompareReference
{
    bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs.m_Distance < rhs.m_Distance;
    }
};

struct SHLightFieldData
{
    Eigen::Matrix4f m_InverseMatrix;
    Eigen::Vector3f m_Position;
    uint32_t m_ProbeCounts[3];
    float m_Distance;
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> m_spPicture;
};

struct IBLProbeData
{
    Eigen::Matrix4f m_InverseMatrix;
    Eigen::Vector3f m_Position;
    float m_Bias;
    float m_Radius;
    float m_Distance;
    boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> m_spPicture;
};

struct LocalLightData
{
    boost::shared_ptr<Hedgehog::Mirage::CLightData> m_spLightData;
    float m_Distance;
};

class RenderDataManager
{
    static bool enabled;

public:
    static boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> ms_spDefaultIBLPicture;
    static boost::shared_ptr<Hedgehog::Yggdrasill::CYggPicture> ms_spRgbTablePicture;

    static std::vector<std::unique_ptr<SHLightFieldData>> ms_SHLFs;
    static std::vector<std::unique_ptr<IBLProbeData>> ms_IBLProbes;

    static std::set<const SHLightFieldData*, DistanceComparePointer<const SHLightFieldData*>> ms_SHLFsInFrustum;
    static std::set<const IBLProbeData*, DistanceComparePointer<const IBLProbeData*>> ms_IBLProbesInFrustum;
    static std::set<LocalLightData, DistanceCompareReference<LocalLightData>> ms_LocalLightsInFrustum;

    static void applyPatches();
};
